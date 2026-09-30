#pragma once
// ---------------------------------------------------------------------------
// inverted_index.h — inverted index build + candidate query (FR-11/12, FR-20)
//
// Build(docs)
//   for each doc i: for each unique term t: index[t].push_back(i)
//   TIME  O(sum U_d)   SPACE O(sum df(t))   (posting lists are built in
//   increasing doc-id order by construction, so they are already sorted)
//
// Query(qterms, min_candidates, max_terms=20) -> candidate doc ids
//   1. Rank query terms by df (ascending) and keep only the `max_terms`
//      RAREST terms — walking only short posting lists keeps the walk at
//      O(sum df(t)) for small-df t, which is what makes the index beat the
//      O(N) brute-force scan (NFR-02).
//   2. Overlap counting: for each kept term, walk its posting list and
//      increment cnt[doc].                                  TIME O(sum df(t))
//   3. Keep docs with count >= threshold, threshold = max(2, |kept|/4),
//      relaxing to 1 (then any match) until at least `min_candidates` docs
//      survive — this is a relaxed AND (SPEC §3.5) so that long resume
//      queries never degenerate to the empty set.
//   4. If NO query term exists in the index the result is empty.
//
//   TIME  O(|q| log|q| + sum df(t_rarest) + N)    SPACE O(N)
// ---------------------------------------------------------------------------
#include <algorithm>
#include <string>
#include <unordered_map>
#include <vector>

#include "document.h"

namespace jre {

class InvertedIndex {
public:
    // Build the term -> doc-id lists. TIME O(sum U_d).
    void build(const std::vector<Document>& docs) {
        index_.clear();
        index_.reserve(4096);
        doc_count_ = static_cast<int>(docs.size());
        for (int i = 0; i < doc_count_; ++i) {
            std::unordered_map<std::string, bool> seen;
            for (const std::string& t : docs[i].tokens) {
                if (seen.insert({t, true}).second) index_[t].push_back(i);
            }
        }
    }

    // Candidate retrieval for a stemmed query. TIME O(|q| log|q| + sum df + N).
    std::vector<int> query(const std::vector<std::string>& qterms,
                           int min_candidates, int max_terms = 20) const {
        std::unordered_map<std::string, bool> distinct;
        for (const std::string& t : qterms) distinct.insert({t, true});
        if (index_.empty() || distinct.empty()) return {};

        // 1. collect posting lists of query terms, smallest df first
        std::vector<const std::vector<int>*> lists;
        lists.reserve(distinct.size());
        for (const auto& kv : distinct) {
            auto it = index_.find(kv.first);
            if (it == index_.end()) continue;
            lists.push_back(&it->second);
        }
        if (lists.empty()) return {};   // no query term appears in the corpus
        std::sort(lists.begin(), lists.end(),
                  [](const auto* a, const auto* b) { return a->size() < b->size(); });
        if (static_cast<int>(lists.size()) > max_terms)
            lists.resize(static_cast<size_t>(max_terms));

        // 2. overlap counting over the kept (rarest) terms
        std::vector<int> cnt(doc_count_, 0);   // cnt[doc] = # distinct terms matched
        for (const std::vector<int>* list : lists)
            for (int doc : *list) cnt[doc] += 1;

        // 3. adaptive threshold over the kept terms
        int total = static_cast<int>(lists.size());
        int threshold = total / 4;
        if (threshold < 2) threshold = 2;
        if (threshold > total) threshold = total;

        std::vector<int> out;
        while (true) {
            out.clear();
            for (int d = 0; d < static_cast<int>(cnt.size()); ++d)
                if (cnt[d] >= threshold) out.push_back(d);
            if (static_cast<int>(out.size()) >= min_candidates || threshold <= 1)
                break;
            --threshold;               // relax the AND condition
        }
        return out;
    }

    size_t term_count() const { return index_.size(); }

private:
    std::unordered_map<std::string, std::vector<int>> index_;
    int doc_count_ = 0;
};

}  // namespace jre
