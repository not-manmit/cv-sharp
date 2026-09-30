#pragma once
// ---------------------------------------------------------------------------
// tokenizer.h — text normalisation, tokenisation, stop-word removal (FR-03/04)
//
// Tokenize(text) -> vector<string>
//   1. Scan text; accumulate lowercase letters/digits/+/#/-/ into a buffer.
//   2. On any other character flush the buffer:
//        - trim leading/trailing '-' hyphens
//        - keep only tokens with length > 2 that are not stop words
//   3. Flush any remaining buffer at end of input.
//
// TIME  O(L) where L = input text length
// SPACE O(T) where T = number of tokens produced
// ---------------------------------------------------------------------------
#include <cctype>
#include <fstream>
#include <string>
#include <unordered_set>
#include <vector>

namespace jre {

// Two-letter tech abbreviations exempt from the FR-04 "length > 2" rule —
// without this the word "AI" vanishes from BOTH resume and job tokens, so
// an AI resume can never match an "AI Engineer" posting.
inline const std::unordered_set<std::string>& short_tech_tokens() {
    static const std::unordered_set<std::string> kTokens = {
        "ai", "ml", "go", "ui", "ux", "ar", "vr", "bi", "db", "3d", "5g",
    };
    return kTokens;
}

class Tokenizer {
public:
    Tokenizer() { stopwords_ = default_stopwords(); }

    // Override the built-in stop-word list with a file (one word per line).
    // Returns false and keeps defaults if the file cannot be read.
    bool load_stopwords(const std::string& path) {
        std::ifstream in(path);
        if (!in) return false;
        std::unordered_set<std::string> words;
        std::string line;
        while (std::getline(in, line)) {
            while (!line.empty() && (line.back() == '\r' || line.back() == '\n' ||
                                     line.back() == ' '))
                line.pop_back();
            if (!line.empty()) words.insert(line);
        }
        if (words.empty()) return false;
        stopwords_ = std::move(words);
        return true;
    }

    // TIME O(L), SPACE O(T)
    std::vector<std::string> tokenize(const std::string& text) const {
        std::vector<std::string> out;
        std::string buf;
        buf.reserve(32);
        auto flush = [&]() {
            // trim hyphens at both ends
            size_t b = 0, e = buf.size();
            while (b < e && buf[b] == '-') ++b;
            while (e > b && buf[e - 1] == '-') --e;
            if (e > b) {
                std::string tok = buf.substr(b, e - b);
                // FR-04 says length > 2; two-letter tech abbreviations that
                // carry real signal (ai, ml, go, ...) are whitelisted so an
                // "AI engineer" resume can actually match "AI Engineer" jobs.
                const bool valid =
                    (e - b > 2) || short_tech_tokens().count(tok) == 1;
                if (valid && stopwords_.find(tok) == stopwords_.end())
                    out.push_back(std::move(tok));
            }
            buf.clear();
        };
        for (unsigned char c : text) {
            if (std::isalnum(c) || c == '+' || c == '#' || c == '-' || c == '_') {
                buf.push_back(static_cast<char>(std::tolower(c)));
            } else {
                flush();
            }
        }
        flush();
        return out;
    }

    const std::unordered_set<std::string>& stopwords() const { return stopwords_; }

private:
    std::unordered_set<std::string> stopwords_;

    // Core English stop words used when data/stopwords.txt is unavailable.
    static std::unordered_set<std::string> default_stopwords() {
        static const char* kWords[] = {
            "the", "and", "for", "are", "but", "not", "you", "all", "any", "can",
            "had", "her", "was", "one", "our", "out", "has", "have", "his", "her",
            "she", "they", "them", "their", "this", "that", "these", "those",
            "with", "from", "into", "than", "then", "them", "been", "were", "will",
            "would", "should", "could", "about", "after", "before", "between",
            "during", "under", "over", "again", "further", "once", "here", "there",
            "when", "where", "why", "how", "what", "which", "who", "whom", "while",
            "only", "own", "same", "very", "just", "because", "as", "until", "while",
            "of", "at", "by", "for", "with", "on", "in", "to", "from", "up", "down",
            "is", "am", "are", "was", "were", "be", "been", "being", "do", "does",
            "did", "doing", "a", "an", "the", "if", "or", "nor", "so", "too",
            "also", "each", "few", "more", "most", "other", "some", "such", "no",
            "s", "t", "y", "now", "get", "got", "via", "per", "etc",
        };
        return std::unordered_set<std::string>(std::begin(kWords), std::end(kWords));
    }
};

}  // namespace jre
