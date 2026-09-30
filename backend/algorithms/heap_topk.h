#pragma once
// ---------------------------------------------------------------------------
// heap_topk.h — top-K selection with a min-heap (FR-15) vs full sort (FR-21)
//
// TopK(scores, k) -> k highest-scoring pairs, descending
//   1. Maintain a min-heap of size k (top = smallest kept score).
//   2. For each pair: push if |H| < k, else replace top if score is larger.
//   3. Drain the heap, reverse -> descending order.
//
// TIME  O(N log K)      SPACE O(K)
// Full std::sort is O(N log N) — the heap wins when K << N (NFR-03).
// ---------------------------------------------------------------------------
#include <algorithm>
#include <functional>
#include <queue>
#include <vector>

#include "document.h"

namespace jre {

inline std::vector<ScoredDoc> topk_min_heap(const std::vector<ScoredDoc>& scores,
                                            int k) {
    if (k <= 0) return {};
    using MinHeap =
        std::priority_queue<ScoredDoc, std::vector<ScoredDoc>,
                            std::function<bool(const ScoredDoc&, const ScoredDoc&)>>;
    MinHeap heap([](const ScoredDoc& a, const ScoredDoc& b) {
        return a.score > b.score;  // smallest score on top
    });
    for (const ScoredDoc& s : scores) {
        if (static_cast<int>(heap.size()) < k) {
            heap.push(s);
        } else if (s.score > heap.top().score) {
            heap.pop();
            heap.push(s);
        }
    }
    std::vector<ScoredDoc> out(heap.size());
    for (int i = static_cast<int>(out.size()) - 1; i >= 0; --i) {
        out[static_cast<size_t>(i)] = heap.top();
        heap.pop();
    }
    return out;  // descending
}

// Reference implementation used by the sort-vs-heap benchmark: O(N log N).
inline std::vector<ScoredDoc> topk_full_sort(std::vector<ScoredDoc> scores,
                                             int k) {
    std::sort(scores.begin(), scores.end(),
              [](const ScoredDoc& a, const ScoredDoc& b) { return a.score > b.score; });
    if (k >= 0 && k < static_cast<int>(scores.size())) scores.resize(k);
    return scores;
}

}  // namespace jre
