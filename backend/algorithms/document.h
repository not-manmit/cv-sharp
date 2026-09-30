#pragma once
// ---------------------------------------------------------------------------
// document.h — shared document / scoring types
// ---------------------------------------------------------------------------
#include <string>
#include <unordered_map>
#include <vector>

namespace jre {

// A processed job posting (SPEC §2.2).
struct Document {
    std::string id;
    std::string title;      // display fields
    std::string company;
    std::string location;
    std::string description;   // original job description (display)
    std::string raw_text;   // normalised concatenation used for tokens
    std::vector<std::string> tokens;                    // tokenised + stemmed
    std::unordered_map<std::string, double> tfidf;      // term -> weight
    std::vector<std::string> skills;                    // original skills list
    std::vector<std::string> skills_lc;                 // lowercased (matching)
    double norm = 0.0;      // L2 norm of tfidf vector
};

// One (score, document-index) pair used by ranking and top-K selection.
struct ScoredDoc {
    double score = 0.0;
    int idx = -1;
};

}  // namespace jre
