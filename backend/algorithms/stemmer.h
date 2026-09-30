#pragma once
// ---------------------------------------------------------------------------
// stemmer.h — full Porter Stemmer (1980), 5 steps, implemented from scratch.
//
// Stem(word) -> string
//   Step 1a: plurals   (sses->ss, ies->i, ss->ss, s->"")
//   Step 1b: past tense / gerund (eed->ee, ed->"", ing->"" + at/bl/iz,
//            double-consonant trim, add-e)
//   Step 1c: y -> i when the stem (word minus trailing y) contains a vowel
//   Step 2:  double suffixes  (ational->ate, tional->tion, izer->ize, ...)
//   Step 3:  further suffixes (icate->ic, ative->"", alize->al, ...)
//   Step 4:  complex suffixes (al, ance, ence, er, ic, able, ant, ment, ion...)
//   Step 5a: final e removal  (m>1, or m==1 and not *d*)
//   Step 5b: final ll -> l    (m>1 and *d*)
//
//   m  = measure = number of VC sequences in the stem (see measure())
//   *d* = stem ends consonant-vowel-consonant, last consonant not w/x/y
//
// Non-alphabetic tokens ("c++", "real-time") are returned unchanged so the
// same tokeniser output maps to itself on both resume and job side.
//
// TIME  O(W) per word (suffix matching is a constant number of comparisons)
// SPACE O(W)
// ---------------------------------------------------------------------------
#include <string>

namespace jre {

class PorterStemmer {
public:
    // Stem a single lowercase token. TIME O(W), SPACE O(W).
    std::string stem(const std::string& w) const {
        if (w.size() <= 2 || !all_letters(w)) return w;
        std::string s = w;
        step1a(s);
        step1b(s);
        step1c(s);
        step2(s);
        step3(s);
        step4(s);
        step5(s);
        return s;
    }

private:
    // -- basic predicates ---------------------------------------------------

    static bool vowel(char c) {
        return c == 'a' || c == 'e' || c == 'i' || c == 'o' || c == 'u';
    }

    // consonant at position i (y is consonant unless preceded by a consonant)
    static bool cons(const std::string& w, int i) {
        char c = w[static_cast<size_t>(i)];
        if (vowel(c)) return false;
        if (c == 'y') return i == 0 ? true : !cons(w, i - 1);
        return true;  // any other letter or non-letter
    }

    static bool all_letters(const std::string& w) {
        for (char c : w)
            if (c < 'a' || c > 'z') return false;
        return true;
    }

    // true if any vowel occurs in w
    static bool has_vowel(const std::string& w) {
        for (size_t i = 0; i < w.size(); ++i)
            if (!cons(w, static_cast<int>(i))) return true;
        return false;
    }

    // m = number of VC sequences in w. TIME O(|w|).
    static int measure(const std::string& w) {
        int n = static_cast<int>(w.size()), i = 0, m = 0;
        while (i < n && cons(w, i)) ++i;          // skip leading C*
        while (i < n) {                            // (VC)*
            while (i < n && !cons(w, i)) ++i;      // V*
            if (i >= n) break;
            ++i;                                   // C
            while (i < n && cons(w, i)) ++i;
            ++m;
        }
        return m;
    }

    // *d* : last three letters form C-V-C with last letter not w/x/y
    static bool ends_cvc(const std::string& w) {
        int n = static_cast<int>(w.size());
        if (n < 3) return false;
        if (cons(w, n - 1) && !cons(w, n - 2) && cons(w, n - 3)) {
            char last = w[static_cast<size_t>(n - 1)];
            return last != 'w' && last != 'x' && last != 'y';
        }
        return false;
    }

    static bool ends_with(const std::string& w, const std::string& suf) {
        return w.size() >= suf.size() &&
               w.compare(w.size() - suf.size(), suf.size(), suf) == 0;
    }

    // Replace suffix `suf` with `rep` if present. Returns true if the suffix
    // matched (regardless of whether the measure condition held).
    static bool match_suffix(std::string& w, const std::string& suf,
                             const std::string& rep, int min_m, bool& applied) {
        applied = false;
        if (!ends_with(w, suf)) return false;
        std::string stem = w.substr(0, w.size() - suf.size());
        if (min_m < 0 || measure(stem) >= min_m) {
            w = stem + rep;
            applied = true;
        }
        return true;  // matched — caller must not try shorter alternatives
    }

    // -- steps --------------------------------------------------------------

    static void step1a(std::string& w) {
        // sses -> ss, ies -> i (drop the trailing "es"), ss -> ss, s -> ""
        if (ends_with(w, "sses"))
            w.resize(w.size() - 2);
        else if (ends_with(w, "ies"))
            w.resize(w.size() - 2);
        else if (ends_with(w, "ss"))
            return;
        else if (ends_with(w, "s"))
            w.resize(w.size() - 1);
    }

    static void step1b(std::string& w) {
        if (ends_with(w, "eed")) {
            std::string stem = w.substr(0, w.size() - 3);
            if (measure(stem) > 0) w = stem + "ee";   // (m>0) eed -> ee
            return;
        }
        bool done = false;
        if (ends_with(w, "ed")) {
            std::string stem = w.substr(0, w.size() - 2);
            if (has_vowel(stem)) { w = stem; done = true; }
        } else if (ends_with(w, "ing")) {
            std::string stem = w.substr(0, w.size() - 3);
            if (has_vowel(stem)) { w = stem; done = true; }
        }
        if (!done) return;
        if (ends_with(w, "at") || ends_with(w, "bl") || ends_with(w, "iz")) {
            w += 'e';                                    // at->ate, bl->ble, iz->ize
        } else if (w.size() >= 2 && w[w.size() - 1] == w[w.size() - 2]) {
            char last = w.back();
            if (last != 'l' && last != 's' && last != 'z' && measure(w) > 1) {
                w.pop_back();                            // doubled consonant
            }
        } else if (measure(w) == 1 && ends_cvc(w)) {
            w += 'e';                                    // m==1 and *d* -> add e
        }
    }

    static void step1c(std::string& w) {
        // (*v*) y -> i : stem (word without trailing y) contains a vowel
        if (ends_with(w, "y")) {
            std::string stem = w.substr(0, w.size() - 1);
            if (has_vowel(stem)) w = stem + 'i';
        }
    }

    static void step2(std::string& w) {
        // Official Porter ordering (longest / most specific first).
        struct Rule { const char* suf; const char* rep; };
        static const Rule kRules[] = {
            {"ational", "ate"}, {"tional", "tion"}, {"enci", "ence"},
            {"anci", "ance"},   {"izer", "ize"},   {"abli", "able"},
            {"alli", "al"},     {"entli", "ent"},  {"eli", "e"},
            {"ousli", "ous"},   {"ization", "ize"}, {"ation", "ate"},
            {"ator", "ate"},    {"alism", "al"},   {"iveness", "ive"},
            {"fulness", "ful"}, {"ousness", "ous"},{"aliti", "al"},
            {"iviti", "ive"},   {"biliti", "ble"}, {"logi", "log"},
        };
        for (const Rule& r : kRules) {
            bool applied = false;
            if (match_suffix(w, r.suf, r.rep, 1, applied)) return;  // (m>0)
        }
    }

    static void step3(std::string& w) {
        struct Rule { const char* suf; const char* rep; };
        static const Rule kRules[] = {
            {"icate", "ic"}, {"ative", ""}, {"alize", "al"},
            {"iciti", "ic"}, {"ical", "ic"}, {"ful", ""}, {"ness", ""},
        };
        for (const Rule& r : kRules) {
            bool applied = false;
            if (match_suffix(w, r.suf, r.rep, 1, applied)) return;  // (m>0)
        }
    }

    static void step4(std::string& w) {
        // (m>1) deletions; "ion" only after s or t.
        if (ends_with(w, "ion")) {
            std::string stem = w.substr(0, w.size() - 3);
            if (measure(stem) > 1 && !stem.empty() &&
                (stem.back() == 's' || stem.back() == 't'))
                w = stem;
            return;
        }
        struct Rule { const char* suf; };
        static const Rule kRules[] = {
            {"al"},   {"ance"}, {"ence"}, {"er"},   {"ic"},   {"able"},
            {"ible"}, {"ant"},  {"ement"}, {"ment"}, {"ent"}, {"ism"},
            {"ate"},  {"iti"},  {"ous"},  {"ive"},  {"ize"},
        };
        for (const Rule& r : kRules) {
            bool applied = false;
            if (match_suffix(w, r.suf, "", 2, applied)) return;  // (m>1)
        }
    }

    static void step5(std::string& w) {
        // 5a: final e removal
        if (ends_with(w, "e")) {
            std::string stem = w.substr(0, w.size() - 1);
            int m = measure(stem);
            if (m > 1 || (m == 1 && !ends_cvc(stem))) w = stem;
        }
        // 5b: (m>1 and *d*) ll -> l
        if (ends_with(w, "ll") && measure(w) > 1) {
            std::string stem = w.substr(0, w.size() - 1);
            if (ends_cvc(stem)) w.pop_back();
        }
    }
};

}  // namespace jre
