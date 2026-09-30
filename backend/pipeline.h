#pragma once
// ---------------------------------------------------------------------------
// pipeline.h — Ranking Pipeline Orchestrator (SPEC §1.2)
//
//   Resume -> Tokenize -> Stem -> TF-IDF --+
//                                          +-> Inverted Index -> Cosine ----+
//   Jobs   -> Tokenize -> Stem -> TF-IDF --+      Skill match (substring) --+-> 0.7*cos+0.3*skill -> Top-K
//
// TIME  (per request)
//   tokenize      O(L_resume)
//   tfidf query   O(T_q)
//   candidates    O(sum df(t) + N)          inverted_index.h
//   scoring       O(|candidates| * avg|v|)
//   top-K         O(|candidates| log K) + O(K) diversity pass heap_topk.h
//   TOTAL         sub-linear in N when the index is selective (NFR-02)
// SPACE O(N) documents + O(total_postings) index (built once at startup)
// ---------------------------------------------------------------------------
#include <algorithm>
#include <cctype>
#include <chrono>
#include <fstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "algorithms/cosine.h"
#include "algorithms/document.h"
#include "algorithms/heap_topk.h"
#include "algorithms/inverted_index.h"
#include "algorithms/stemmer.h"
#include "algorithms/tfidf.h"
#include "algorithms/tokenizer.h"
#include "third_party/nlohmann/json.hpp"

namespace jre {

using json = nlohmann::json;

// Combined scoring weights (SPEC §3.10 / PRD FR-19).
constexpr double kAlphaCosine = 0.7;   // TF-IDF cosine weight
constexpr double kBetaSkill = 0.3;     // exact skill-match weight

inline double now_ms() {
    using clock = std::chrono::steady_clock;
    return std::chrono::duration<double, std::milli>(clock::now().time_since_epoch())
        .count();
}

class RankingEngine {
public:
    struct StageTimings {
        double tokenize_ms = 0, tfidf_ms = 0, candidate_filter_ms = 0;
        double scoring_ms = 0, topk_select_ms = 0, total_ms = 0;
    };
    struct RankedJob {
        int rank = 0;
        std::string job_id, title, company, location, description;
        double cosine_score = 0.0, skill_match_score = 0.0, combined_score = 0.0;
        std::vector<std::string> matched_terms;
    };
    struct RankResponse {
        StageTimings timings;
        int total_jobs = 0, candidates = 0;
        std::vector<RankedJob> results;
    };

    // --- data loading (FR-06/07) ------------------------------------------
    // Parse jobs.json, normalise + tokenise every posting (FR-09).
    // TIME O(corpus text length), SPACE O(corpus).
    bool load_jobs(const std::string& path, std::string& err) {
        std::ifstream in(path);
        if (!in) { err = "cannot open jobs file: " + path; return false; }
        json j;
        try {
            in >> j;
        } catch (const std::exception& e) {
            err = std::string("invalid jobs JSON: ") + e.what();
            return false;
        }
        if (!j.is_array()) { err = "jobs JSON must be an array"; return false; }
        docs_.clear();
        docs_.reserve(j.size());
        skill_vocab_.clear();
        for (const auto& item : j) {
            Document d;
            d.id = item.value("id", "");
            d.title = item.value("title", "");
            d.company = item.value("company", "");
            d.location = item.value("location", "");
            std::string desc = item.value("description", "");
            d.description = desc;
            d.raw_text = d.title + " . " + desc + " . skills: ";
            if (item.contains("skills") && item["skills"].is_array()) {
                for (const auto& s : item["skills"]) {
                    std::string sk = s.get<std::string>();
                    d.skills.push_back(sk);
                    std::string lc = to_lower(sk);
                    skill_vocab_.insert(lc);          // global skill vocabulary
                    d.skills_lc.push_back(std::move(lc));
                    d.raw_text += sk + ", ";
                }
            }
            d.tokens = tokenize_stem(d.raw_text);
            docs_.push_back(std::move(d));
        }
        return true;
    }

    bool load_stopwords(const std::string& path) {
        return tokenizer_.load_stopwords(path);
    }

    // Build DF/TF-IDF vectors + inverted index once at startup (FR-12).
    // TIME O(total_tokens), SPACE O(V + postings).
    void build() {
        model_.build_df(docs_);
        model_.embed_all(docs_);
        index_.build(docs_);
    }

    // --- ranking (FR-14..FR-16, FR-19) -------------------------------------
    RankResponse rank(const std::string& resume_text, int top_k) const {
        RankResponse resp;
        if (top_k <= 0) top_k = 10;
        resp.total_jobs = static_cast<int>(docs_.size());
        const double t0 = now_ms();

        // 1. tokenise + stem the resume (FR-03/04/05)
        std::vector<std::string> raw = tokenizer_.tokenize(resume_text);
        std::vector<std::string> qtokens;
        std::unordered_map<std::string, std::string> stem_display;
        qtokens.reserve(raw.size());
        for (const std::string& t : raw) {
            std::string s = stemmer_.stem(t);
            stem_display.emplace(s, t);
            qtokens.push_back(std::move(s));
        }
        const double t1 = now_ms();

        // 2. query TF-IDF vector (same model as the corpus)
        Document q;
        q.tokens = qtokens;
        model_.embed(q);
        const double t2 = now_ms();

        // 3. candidate retrieval through the inverted index (FR-11)
        std::vector<int> cand = index_.query(qtokens, std::max(top_k, 10));
        resp.candidates = static_cast<int>(cand.size());
        const double t3 = now_ms();

        // 4. cosine + exact skill match -> combined score (FR-14, FR-19)
        //    Skill signal: which entries of the corpus skill vocabulary occur
        //    in the resume text (word-boundary aware). Then for each candidate
        //    skill_score = matched_job_skills / total_job_skills.
        //    combined = alpha * cosine + beta * skill_score   (SPEC §3.10)
        const std::string resume_lc = to_lower(resume_text);
        std::unordered_map<std::string, bool> resume_skills;
        resume_skills.reserve(skill_vocab_.size() * 2);
        for (const std::string& skill : skill_vocab_)
            if (contains_phrase(resume_lc, skill)) resume_skills[skill] = true;

        std::vector<ScoredDoc> scores;
        scores.reserve(cand.size());
        for (int i : cand) {
            const Document& d = docs_[static_cast<size_t>(i)];
            const double cosine =
                cosine_similarity(q.tfidf, q.norm, d.tfidf, d.norm);
            const double skill = skill_score(d, resume_skills);
            const double combined = kAlphaCosine * cosine + kBetaSkill * skill;
            scores.push_back({combined, i});
        }
        const double t4 = now_ms();

        // 5. min-heap top-K over a widened pool, then title-diversity pass
        //    (fixes repetitive results: at most kMaxPerTitle postings may
        //    share an identical title; equal-score duplicates otherwise win
        //    every tie and fill the whole list)
        constexpr int kMaxPerTitle = 1;
        const int pool_size =
            std::min(static_cast<int>(scores.size()), top_k * 4 + 20);
        std::vector<ScoredDoc> pool = topk_min_heap(scores, pool_size);
        std::vector<ScoredDoc> top;
        top.reserve(static_cast<size_t>(top_k));
        std::vector<char> used(pool.size(), 0);
        std::unordered_map<std::string, int> per_title;
        for (size_t i = 0; i < pool.size(); ++i) {
            const std::string& title = docs_[pool[i].idx].title;
            if (per_title[title] < kMaxPerTitle) {
                per_title[title] += 1;
                used[i] = 1;
                top.push_back(pool[i]);
                if (static_cast<int>(top.size()) >= top_k) break;
            }
        }
        // fallback: if distinct titles ran out (large K), fill from the pool
        if (static_cast<int>(top.size()) < top_k) {
            for (size_t i = 0; i < pool.size() &&
                              static_cast<int>(top.size()) < top_k;
                 ++i) {
                if (!used[i]) {
                    used[i] = 1;
                    top.push_back(pool[i]);
                }
            }
        }
        const double t5 = now_ms();

        // 6. build response with matched terms (FR-28)
        //    (recompute the components for the K survivors — cheap)
        resp.results.reserve(top.size());
        int rank = 1;
        for (const ScoredDoc& sd : top) {
            const Document& d = docs_[static_cast<size_t>(sd.idx)];
            RankedJob rj;
            rj.rank = rank++;
            rj.job_id = d.id;
            rj.title = d.title;
            rj.company = d.company;
            rj.location = d.location;
            rj.description = d.description;
            rj.cosine_score =
                round3(cosine_similarity(q.tfidf, q.norm, d.tfidf, d.norm));
            rj.skill_match_score = round3(skill_score(d, resume_skills));
            rj.combined_score = round3(kAlphaCosine * rj.cosine_score +
                                       kBetaSkill * rj.skill_match_score);
            rj.matched_terms = matched_terms(q, d, stem_display, 6);
            resp.results.push_back(std::move(rj));
        }

        resp.timings.tokenize_ms = round2(t1 - t0);
        resp.timings.tfidf_ms = round2(t2 - t1);
        resp.timings.candidate_filter_ms = round2(t3 - t2);
        resp.timings.scoring_ms = round2(t4 - t3);
        resp.timings.topk_select_ms = round2(t5 - t4);
        resp.timings.total_ms = round2(t5 - t0);
        return resp;
    }

    json rank_json(const std::string& resume_text, int top_k) const {
        RankResponse r = rank(resume_text, top_k);
        json out;
        out["status"] = "success";
        out["timings"] = {
            {"tokenize_ms", r.timings.tokenize_ms},
            {"tfidf_ms", r.timings.tfidf_ms},
            {"candidate_filter_ms", r.timings.candidate_filter_ms},
            {"scoring_ms", r.timings.scoring_ms},
            {"topk_select_ms", r.timings.topk_select_ms},
            {"total_ms", r.timings.total_ms},
        };
        out["total_jobs_considered"] = r.total_jobs;
        out["candidates_after_filter"] = r.candidates;
        out["results"] = json::array();
        for (const RankedJob& j : r.results) {
            out["results"].push_back({
                {"rank", j.rank},
                {"job_id", j.job_id},
                {"title", j.title},
                {"company", j.company},
                {"location", j.location},
                {"description", j.description},
                {"cosine_score", j.cosine_score},
                {"skill_match_score", j.skill_match_score},
                {"combined_score", j.combined_score},
                {"matched_terms", j.matched_terms},
            });
        }
        return out;
    }

    const std::vector<Document>& docs() const { return docs_; }
    size_t index_term_count() const { return index_.term_count(); }

    // Public so the benchmark engine can reuse the exact same query path.
    std::vector<std::string> tokenize_stem(const std::string& text) const {
        std::vector<std::string> raw = tokenizer_.tokenize(text);
        std::vector<std::string> out;
        out.reserve(raw.size());
        for (const std::string& t : raw) out.push_back(stemmer_.stem(t));
        return out;
    }
    const TfIdfModel& model() const { return model_; }
    const InvertedIndex& index() const { return index_; }
    Tokenizer& tokenizer() { return tokenizer_; }

private:
    std::vector<Document> docs_;
    Tokenizer tokenizer_;
    PorterStemmer stemmer_;
    TfIdfModel model_;
    InvertedIndex index_;
    std::unordered_set<std::string> skill_vocab_;   // union of all job skills

    static std::string to_lower(std::string s) {
        for (char& c : s)
            c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        return s;
    }

    // Word-boundary-aware substring test. Both arguments must be lowercase.
    // "java" does NOT match "javascript"; "c++" matches "…with C++, Python…".
    // TIME O(|hay|) naive scan.
    static bool contains_phrase(const std::string& hay, const std::string& needle) {
        if (needle.empty() || hay.size() < needle.size()) return false;
        size_t pos = 0;
        while ((pos = hay.find(needle, pos)) != std::string::npos) {
            const bool left_ok =
                pos == 0 || !std::isalnum(static_cast<unsigned char>(hay[pos - 1]));
            const size_t end = pos + needle.size();
            const bool right_ok =
                end >= hay.size() ||
                !std::isalnum(static_cast<unsigned char>(hay[end]));
            if (left_ok && right_ok) return true;
            ++pos;
        }
        return false;
    }

    // Fraction of this job's skills present in the resume (FR-19 skill term).
    // TIME O(|job.skills|)
    static double skill_score(const Document& d,
                              const std::unordered_map<std::string, bool>& resume_skills) {
        if (d.skills_lc.empty()) return 0.0;
        int matched = 0;
        for (const std::string& s : d.skills_lc)
            if (resume_skills.find(s) != resume_skills.end()) ++matched;
        return static_cast<double>(matched) / static_cast<double>(d.skills_lc.size());
    }

    static double round3(double v) { return static_cast<double>(static_cast<int>(v * 1000 + (v < 0 ? -0.5 : 0.5))) / 1000.0; }
    static double round2(double v) { return static_cast<double>(static_cast<int>(v * 100 + (v < 0 ? -0.5 : 0.5))) / 100.0; }

    // Terms where both vectors have weight, ranked by weight product (FR-28).
    static std::vector<std::string> matched_terms(
        const Document& q, const Document& d,
        const std::unordered_map<std::string, std::string>& stem_display,
        int limit) {
        std::vector<std::pair<double, std::string>> shared;
        for (const auto& kv : q.tfidf) {
            auto it = d.tfidf.find(kv.first);
            if (it == d.tfidf.end()) continue;
            auto disp = stem_display.find(kv.first);
            shared.push_back({kv.second * it->second,
                              disp == stem_display.end() ? kv.first : disp->second});
        }
        std::sort(shared.begin(), shared.end(),
                  [](const auto& a, const auto& b) { return a.first > b.first; });
        std::vector<std::string> out;
        for (int i = 0; i < limit && i < static_cast<int>(shared.size()); ++i)
            out.push_back(shared[static_cast<size_t>(i)].second);
        return out;
    }
};

}  // namespace jre
