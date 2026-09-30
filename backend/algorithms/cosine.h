#pragma once
// ---------------------------------------------------------------------------
// cosine.h — sparse cosine similarity between TF-IDF vectors (FR-14)
//
// Cosine(v1, n1, v2, n2) -> double in [0, 1]
//   dot  = sum over v1 of w * v2[t]        (iterate the smaller vector)
//   norm1, norm2 precomputed (L2), so only dot is computed per call
//   return 0 if either vector is empty, else dot / (n1 * n2)
//
// TIME  O(min(|v1|, |v2|)) with precomputed norms
// SPACE O(1)
// ---------------------------------------------------------------------------
#include <cmath>
#include <string>
#include <unordered_map>

namespace jre {

using SparseVector = std::unordered_map<std::string, double>;

inline double cosine_similarity(const SparseVector& v1, double n1,
                                const SparseVector& v2, double n2) {
    if (v1.empty() || v2.empty() || n1 <= 0.0 || n2 <= 0.0) return 0.0;
    const SparseVector& small = v1.size() <= v2.size() ? v1 : v2;
    const SparseVector& big = v1.size() <= v2.size() ? v2 : v1;
    double dot = 0.0;
    for (const auto& kv : small) {
        auto it = big.find(kv.first);
        if (it != big.end()) dot += kv.second * it->second;
    }
    double sim = dot / (n1 * n2);
    if (sim < 0.0) return 0.0;
    return sim > 1.0 ? 1.0 : sim;
}

}  // namespace jre
