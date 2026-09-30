#pragma once
// ---------------------------------------------------------------------------
// tfidf.h — TF-IDF vectoriser (FR-10)
//
// BuildDF(docs) -> df map
//   For each doc: for each unique token: document_frequency[token]++
//   TIME O(total_tokens)   SPACE O(V)
//
// Embed(doc) -> doc.tfidf populated, doc.norm computed
//   tf[t]  = count(t) / |doc.tokens|
//   idf[t] = max( ln(N / (1 + df[t])) , 0 )     (clamped so weights stay >= 0)
//   tfidf[t] = tf * idf ;  norm = sqrt(sum w^2)
//   TIME O(T_d)  SPACE O(U_d)
// ---------------------------------------------------------------------------
#include <cmath>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "document.h"

namespace jre {

class TfIdfModel {
public:
    // Build document frequencies over the corpus. TIME O(total_tokens).
    void build_df(const std::vector<Document>& docs) {
        df_.clear();
        df_.reserve(4096);
        for (const Document& d : docs) {
            std::unordered_set<std::string> seen;
            seen.reserve(d.tokens.size() * 2);
            for (const std::string& t : d.tokens) {
                if (seen.insert(t).second) df_[t]++;   // count each term once/doc
            }
        }
        N_ = static_cast<int>(docs.size());
    }

    // Embed one document's token list into a sparse TF-IDF vector.
    // TIME O(T_d)  SPACE O(U_d)
    void embed(Document& d) const {
        d.tfidf.clear();
        if (d.tokens.empty() || N_ == 0) { d.norm = 0.0; return; }
        std::unordered_map<std::string, int> tf;
        tf.reserve(d.tokens.size() * 2);
        for (const std::string& t : d.tokens) tf[t]++;
        const double len = static_cast<double>(d.tokens.size());
        double sum_sq = 0.0;
        d.tfidf.reserve(tf.size() * 2);
        for (const auto& kv : tf) {
            auto it = df_.find(kv.first);
            const int df = it == df_.end() ? 0 : it->second;
            double idf = std::log(static_cast<double>(N_) / (1.0 + df));
            if (idf < 0.0) idf = 0.0;                      // clamp (SPEC §3.3)
            double w = (kv.second / len) * idf;
            if (w > 0.0) {
                d.tfidf[kv.first] = w;
                sum_sq += w * w;
            }
        }
        d.norm = std::sqrt(sum_sq);
    }

    // Embed every document (used once at startup). TIME O(total_tokens).
    void embed_all(std::vector<Document>& docs) const {
        for (Document& d : docs) embed(d);
    }

    int corpus_size() const { return N_; }
    int df(const std::string& term) const {
        auto it = df_.find(term);
        return it == df_.end() ? 0 : it->second;
    }

private:
    std::unordered_map<std::string, int> df_;
    int N_ = 0;
};

}  // namespace jre
