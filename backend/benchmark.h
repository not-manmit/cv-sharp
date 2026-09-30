#pragma once
// ---------------------------------------------------------------------------
// benchmark.h — timing harness + the two V1 experiments (FR-20/21, FR-24)
//
//   1. brute_vs_index  : score all N jobs vs inverted-index candidate filter
//   2. sort_vs_heap    : full std::sort top-K vs min-heap top-K
//
// Rigour (SPEC §9.3): 10 warm-up iterations discarded, `iters` timed runs,
// report median + interquartile range (p25/p75).
// ---------------------------------------------------------------------------
#include <algorithm>
#include <random>
#include <string>
#include <vector>

#include "pipeline.h"

namespace jre {
namespace bench {

// One row of the benchmark table.
struct ExperimentResult {
    std::string name;
    std::string description;
    std::vector<std::string> series;   // algorithm labels (FR-24)
    std::vector<int> ns;
    std::vector<int> ks;               // K per point (0 = n/a)
    std::vector<std::vector<double>> median;  // [series][point]
    std::vector<std::vector<double>> p25;
    std::vector<std::vector<double>> p75;
};

struct Stat {
    double med = 0, p25 = 0, p75 = 0;
};

// Run fn() `iters` times after `warmup` discarded runs. Returns samples (ms).
template <class F>
std::vector<double> time_samples(int warmup, int iters, F&& fn) {
    for (int i = 0; i < warmup; ++i) fn();
    std::vector<double> samples;
    samples.reserve(static_cast<size_t>(iters));
    for (int i = 0; i < iters; ++i) {
        double t0 = now_ms();
        fn();
        samples.push_back(now_ms() - t0);
    }
    return samples;
}

// Median / quartiles of a sample vector. TIME O(I log I).
inline Stat stats(std::vector<double> v) {
    Stat s;
    if (v.empty()) return s;
    std::sort(v.begin(), v.end());
    s.med = v[v.size() / 2];
    s.p25 = v[v.size() / 4];
    s.p75 = v[(v.size() * 3) / 4];
    return s;
}

// Default N ladder, filtered to <= max_n (always includes max_n itself).
inline std::vector<int> ns_up_to(int max_n) {
    const int base[] = {100, 250, 500, 1000, 2500, 5000, 10000};
    std::vector<int> ns;
    for (int n : base)
        if (n <= max_n) ns.push_back(n);
    if (ns.empty() || ns.back() != max_n) ns.push_back(max_n);
    return ns;
}

// ---- Experiment 1: brute force vs inverted index (FR-20) --------------------
// Brute: cosine against all N docs. Index: query index -> cosine on candidates.
// Both finish with the same min-heap top-K, so only retrieval differs.
inline ExperimentResult exp_brute_vs_index(const std::vector<Document>& docs,
                                           const Document& q,
                                           const std::vector<std::string>& qtokens,
                                           int max_n, int k, int iters) {
    ExperimentResult r;
    r.name = "brute_vs_index";
    r.description =
        "Candidate retrieval: scan every job (O(N)) vs inverted-index filter "
        "(O(sum df(t)))";
    r.series = {"brute_force", "inverted_index"};
    r.ns = ns_up_to(max_n);
    r.ks.assign(r.ns.size(), k);
    r.median.assign(2, {});
    r.p25.assign(2, {});
    r.p75.assign(2, {});

    for (size_t ni = 0; ni < r.ns.size(); ++ni) {
        const int N = r.ns[ni];
        std::vector<Document> sub(docs.begin(), docs.begin() + N);
        InvertedIndex idx;
        idx.build(sub);   // build outside the timed region (query-time only)

        auto brute = [&]() {
            std::vector<ScoredDoc> sc;
            sc.reserve(static_cast<size_t>(N));
            for (int i = 0; i < N; ++i)
                sc.push_back({cosine_similarity(q.tfidf, q.norm, sub[static_cast<size_t>(i)].tfidf,
                                                sub[static_cast<size_t>(i)].norm), i});
            topk_min_heap(sc, k);
        };
        auto via_index = [&]() {
            std::vector<int> cand = idx.query(qtokens, std::max(k, 10));
            std::vector<ScoredDoc> sc;
            sc.reserve(cand.size());
            for (int i : cand)
                sc.push_back({cosine_similarity(q.tfidf, q.norm, sub[static_cast<size_t>(i)].tfidf,
                                                sub[static_cast<size_t>(i)].norm), i});
            topk_min_heap(sc, k);
        };

        Stat sb = stats(time_samples(10, iters, brute));
        Stat si = stats(time_samples(10, iters, via_index));
        r.median[0].push_back(sb.med); r.p25[0].push_back(sb.p25); r.p75[0].push_back(sb.p75);
        r.median[1].push_back(si.med); r.p25[1].push_back(si.p25); r.p75[1].push_back(si.p75);
    }
    return r;
}

// ---- Experiment 2: full sort vs min-heap top-K (FR-21) ---------------------
// Both operate on the same random score array (seeded, reproducible).
inline ExperimentResult exp_sort_vs_heap(int max_n, int k, int iters) {
    ExperimentResult r;
    r.name = "sort_vs_heap";
    r.description = "Top-K selection: full sort O(N log N) vs min-heap O(N log K)";
    r.series = {"full_sort", "min_heap"};
    r.ns = ns_up_to(max_n);
    r.ks.assign(r.ns.size(), k);
    r.median.assign(2, {});
    r.p25.assign(2, {});
    r.p75.assign(2, {});

    for (size_t ni = 0; ni < r.ns.size(); ++ni) {
        const int N = r.ns[ni];
        std::mt19937 rng(42 + static_cast<unsigned>(N));   // deterministic
        std::uniform_real_distribution<double> dist(0.0, 1.0);
        std::vector<ScoredDoc> scores(static_cast<size_t>(N));
        for (int i = 0; i < N; ++i)
            scores[static_cast<size_t>(i)] = {dist(rng), i};

        auto sort_fn = [&]() {
            std::vector<ScoredDoc> v = scores;   // sort needs its own copy
            std::sort(v.begin(), v.end(), [](const ScoredDoc& a, const ScoredDoc& b) {
                return a.score > b.score;
            });
            if (k < static_cast<int>(v.size())) v.resize(static_cast<size_t>(k));
        };
        auto heap_fn = [&]() { topk_min_heap(scores, k); };

        Stat ss = stats(time_samples(10, iters, sort_fn));
        Stat sh = stats(time_samples(10, iters, heap_fn));
        r.median[0].push_back(ss.med); r.p25[0].push_back(ss.p25); r.p75[0].push_back(ss.p75);
        r.median[1].push_back(sh.med); r.p25[1].push_back(sh.p25); r.p75[1].push_back(sh.p75);
    }
    return r;
}

// ---- Serialisation ---------------------------------------------------------

// CSV per SPEC §9.2: experiment,N,K,algorithm,time_ms_median,p25,p75
inline std::string to_csv(const std::vector<ExperimentResult>& all) {
    std::string out = "experiment,N,K,algorithm,time_ms_median,time_ms_p25,time_ms_p75\n";
    char buf[160];
    for (const ExperimentResult& e : all) {
        for (size_t s = 0; s < e.series.size(); ++s) {
            for (size_t i = 0; i < e.ns.size(); ++i) {
                snprintf(buf, sizeof(buf), "%s,%d,%d,%s,%.3f,%.3f,%.3f\n",
                         e.name.c_str(), e.ns[i], e.ks[i], e.series[s].c_str(),
                         e.median[s][i], e.p25[s][i], e.p75[s][i]);
                out += buf;
            }
        }
    }
    return out;
}

// JSON per SPEC §2.4 (data rows keyed by "<series>_ms").
inline json to_json(const std::vector<ExperimentResult>& all) {
    json exps = json::array();
    for (const ExperimentResult& e : all) {
        json data = json::array();
        for (size_t i = 0; i < e.ns.size(); ++i) {
            json row;
            row["N"] = e.ns[i];
            if (e.ks[i] > 0) row["K"] = e.ks[i];
            for (size_t s = 0; s < e.series.size(); ++s)
                row[e.series[s] + "_ms"] = e.median[s][i];
            data.push_back(row);
        }
        exps.push_back({{"name", e.name},
                        {"description", e.description},
                        {"data", data}});
    }
    return {{"status", "success"}, {"experiments", exps}};
}

}  // namespace bench
}  // namespace jre
