# Technical Specification
# Job Ranking Engine — Resume-to-Job Matching System

| Field          | Value                              |
|----------------|------------------------------------|
| Project Name   | Job Ranking Engine                 |
| Version        | 1.0                                |
| Author         | [Your Name]                        |
| Date           | [Today's Date]                     |
| Depends On     | PRD.md v1.0                        |

---

## Table of Contents

1. Architecture Overview
2. Data Models
3. Algorithm Specifications
4. API Specification
5. Frontend Specification
6. Project Structure
7. Build & Run Instructions
8. Testing Strategy
9. Benchmark Specification
10. Quality Evaluation
11. Dependencies

---

## 1. Architecture Overview

### 1.1 System Diagram

```
+------------------------------------------------------------------+
|                        CLIENT (Browser)                           |
|                                                                   |
|  +-------------+   +--------------+   +-----------------------+   |
|  | ResumeUpload |   |  RankedList  |   | BenchmarkDashboard    |   |
|  | Component    |   |  Component   |   | Component             |   |
|  +------+-------+   +------^-------+   +-----------^-----------+   |
|         |                  |                       |              |
|         +------------------+-----------------------+              |
|                            |  HTTP/JSON (fetch API)               |
+----------------------------+------------------------------------+
                             |
                    Port 8080 | Port 3000 (dev) / 80 (prod)
                             |
+----------------------------+------------------------------------+
|                     C++ REST API SERVER                           |
|                            |                                     |
|  +-------------------------+----------------------------------+  |
|  |                Route Handlers (httplib)                     |  |
|  |  POST /api/rank    GET /api/jobs   GET /api/benchmark      |  |
|  +-------------------------+----------------------------------+  |
|                            |                                     |
|  +-------------------------+----------------------------------+  |
|  |            Ranking Pipeline Orchestrator                    |  |
|  |                                                             |  |
|  |  Resume --> Tokenize --> Stem --> TF-IDF --+               |  |
|  |                                             |               |  |
|  |  Jobs   --> Tokenize --> Stem --> TF-IDF ---+-> Inverted   |  |
|  |                                             |    Index      |  |
|  |                                             |   (filter)    |  |
|  |                                             v               |  |
|  |                                        Cosine Sim           |  |
|  |                                        + Skill Match (KMP)  |  |
|  |                                        + Fuzzy Match (Lev)  |  |
|  |                                             |               |  |
|  |                                             v               |  |
|  |                                        Combined Score       |  |
|  |                                             |               |  |
|  |                                             v               |  |
|  |                                        Min-Heap Top-K       |  |
|  +-------------------------------------------------------------+  |
|                                                                    |
|  +-------------------------------------------------------------+  |
|  |                     Algorithm Modules                        |  |
|  |  tokenizer  stemmer  tfidf  inverted_index  cosine          |  |
|  |  heap_topk  kmp      levenshtein  bm25                       |  |
|  +-------------------------------------------------------------+  |
|                                                                    |
|  +-------------------------------------------------------------+  |
|  |                     Benchmark Engine                         |  |
|  |  Runs comparisons, collects timings, exports CSV/JSON        |  |
|  +-------------------------------------------------------------+  |
|                                                                    |
|  +-------------------------------------------------------------+  |
|  |                     Data Layer                               |  |
|  |  jobs.json  |  stopwords.txt  |  skills_taxonomy.json        |  |
|  +-------------------------------------------------------------+  |
+--------------------------------------------------------------------+
```

### 1.2 Data Flow (Ranking Request)

```
1.  User pastes resume text / uploads PDF
2.  Frontend sends POST /api/rank { resume_text, top_k }
3.  Backend tokenises resume --> stems --> computes TF-IDF vector
4.  Backend queries inverted index --> candidate job set
5.  For each candidate job:
      a. Compute cosine similarity (TF-IDF vectors)
      b. Compute skill match score (KMP + Levenshtein)
      c. Combine: final_score = alpha*cosine + beta*skill_score
6.  Feed all scores into min-heap --> extract top-K
7.  Return JSON with ranked jobs + timing breakdown
8.  Frontend renders ranked cards + timing info
```

---

## 2. Data Models

### 2.1 Job Posting (jobs.json)

```json
{
  "id": "job_00001",
  "title": "Senior Software Engineer",
  "company": "TechCorp",
  "location": "San Francisco, CA",
  "description": "We are looking for a senior software engineer with experience in C++, distributed systems, and cloud infrastructure...",
  "skills": ["C++", "distributed systems", "cloud", "Linux", "Python"],
  "experience_years": 5,
  "source": "kaggle_linkedin"
}
```

### 2.2 Processed Document (Internal)

```cpp
struct Document {
    std::string id;
    std::string title;                           // original title (display)
    std::string raw_text;                        // normalised concatenation
    std::vector<std::string> tokens;             // tokenised + stemmed
    std::unordered_map<std::string, double> tfidf_vector; // term -> weight
    std::vector<std::string> skills;             // original skills list
    double norm;                                 // L2 norm of tfidf_vector
};
```

### 2.3 Ranking Result (API Response)

```json
{
  "status": "success",
  "timings": {
    "tokenize_ms": 1.2,
    "tfidf_ms": 3.5,
    "candidate_filter_ms": 2.1,
    "scoring_ms": 45.3,
    "topk_select_ms": 0.8,
    "total_ms": 52.9
  },
  "total_jobs_considered": 10000,
  "candidates_after_filter": 2340,
  "results": [
    {
      "rank": 1,
      "job_id": "job_04521",
      "title": "Senior Software Engineer",
      "company": "TechCorp",
      "location": "San Francisco, CA",
      "cosine_score": 0.847,
      "skill_match_score": 0.720,
      "combined_score": 0.808,
      "matched_terms": ["software", "engineer", "c++", "cloud", "linux"],
      "matched_skills": ["C++", "Linux", "cloud"]
    }
  ]
}
```

### 2.4 Benchmark Result (API Response)

```json
{
  "status": "success",
  "experiments": [
    {
      "name": "brute_force_vs_inverted_index",
      "description": "Candidate retrieval comparison",
      "data": [
        { "N": 100,   "brute_force_ms": 12.3,  "inverted_index_ms": 3.1 },
        { "N": 500,   "brute_force_ms": 61.7,  "inverted_index_ms": 15.2 },
        { "N": 1000,  "brute_force_ms": 123.4, "inverted_index_ms": 29.8 }
      ]
    },
    {
      "name": "sort_vs_heap_topk",
      "description": "Top-K selection comparison",
      "data": [
        { "N": 100,   "sort_ms": 0.8, "heap_ms": 0.3 },
        { "N": 500,   "sort_ms": 4.2, "heap_ms": 1.4 }
      ]
    }
  ]
}
```

### 2.5 Ground Truth (evaluation/ground_truth.json)

```json
[
  {
    "resume_id": "resume_001",
    "ideal_rankings": ["job_04521", "job_00234", "job_07891", "job_01234", "job_05678"],
    "labelled_by": "manual",
    "notes": "Backend engineer resume; matches distributed systems roles"
  }
]
```

---

## 3. Algorithm Specifications

### 3.1 Tokenizer

```
ALGORITHM: Tokenize(text: string) -> vector<string>
---------------------------------------------------
INPUT:  Raw text string (UTF-8, English)
OUTPUT: Vector of lowercase alphabetic tokens (length > 2)

PROCEDURE:
  1. Initialise empty result vector
  2. For each character c in text:
       If c is alphabetic or hyphen:
           Append tolower(c) to current word buffer
       Else:
           If buffer length > 2: push buffer to result
           Clear buffer
  3. If buffer length > 2 at end: push to result
  4. Return result

TIME COMPLEXITY:  O(L)  where L = length of input text
SPACE COMPLEXITY: O(T)  where T = number of tokens
```

### 3.2 Porter Stemmer

```
ALGORITHM: Stem(word: string) -> string
---------------------------------------------------
INPUT:  A single lowercase English word
OUTPUT: Stemmed word

PROCEDURE: (Full Porter Stemmer -- 5 steps)
  Step 1a: Plurals (sses->ss, ies->i, ss->ss, s->null)
  Step 1b: Past tense / gerund (eed->ee, ed->null, ing->null + special cases)
  Step 1c: y -> i (if preceded by consonant and word longer than 2)
  Step 2:  Double suffixes (ational->ate, tional->tion, izer->ize, ...)
  Step 3:  Further suffixes (icate->ic, ative->null, alize->al, ...)
  Step 4:  Complex suffixes (al, ance, ence, er, ic, able, ...)
  Step 5a: Final e removal (if measure > 1, or measure == 1 and not cvc)
  Step 5b: Final double consonant ll -> l (if measure > 1)

TIME COMPLEXITY:  O(W)  where W = word length
SPACE COMPLEXITY: O(W)
```

### 3.3 TF-IDF Vectoriser

```
ALGORITHM: BuildIDF(docs: vector<Document>) -> map<string, int>
---------------------------------------------------
INPUT:  Corpus of N documents (already tokenised)
OUTPUT: document_frequency map (term -> number of docs containing it)

PROCEDURE:
  1. For each document d in docs:
       unique_terms <- set of tokens in d
       For each term t in unique_terms:
           document_frequency[t]++
  2. Return document_frequency

TIME COMPLEXITY:  O(total_tokens)  in worst case
SPACE COMPLEXITY: O(V)  where V = vocabulary size

---

ALGORITHM: ComputeTFIDF(doc: Document, df: map<string,int>, N: int) -> void
---------------------------------------------------
INPUT:  Document with token list, DF map, total document count N
OUTPUT: doc.tfidf_vector populated

PROCEDURE:
  1. Count term frequencies: tf[t] = count of t in doc.tokens
  2. For each term t in tf:
       term_freq <- tf[t] / doc.tokens.size()
       idf <- log(N / (1 + df[t]))
       doc.tfidf_vector[t] <- term_freq * idf
  3. Compute doc.norm <- sqrt(sum of (tfidf_vector[t])^2)

TIME COMPLEXITY:  O(T_d)  where T_d = tokens in doc
SPACE COMPLEXITY: O(U_d)  where U_d = unique terms in doc

TOTAL BUILD COMPLEXITY: O(sum of T_d) = O(total_tokens_in_corpus)
```

### 3.4 BM25 Scorer (Alternative)

```
ALGORITHM: BM25Score(query_tokens, doc, df, N, avg_dl, k1, b) -> double
---------------------------------------------------
PARAMETERS: k1 = 1.2, b = 0.75 (tunable experimentally)

PROCEDURE:
  1. score <- 0
  2. For each term t in query_tokens:
       f <- term frequency of t in doc
       idf <- log((N - df[t] + 0.5) / (df[t] + 0.5) + 1)
       score += idf * (f * (k1 + 1)) / (f + k1 * (1 - b + b * |doc| / avg_dl))
  3. Return score

TIME COMPLEXITY:  O(|query_tokens|)  per document
TOTAL FOR ALL DOCS: O(|query_tokens| * N)
```

### 3.5 Inverted Index

```
ALGORITHM: BuildInvertedIndex(docs: vector<Document>) -> map<string, vector<int>>
---------------------------------------------------
INPUT:  Corpus of documents
OUTPUT: Inverted index: term -> list of document IDs containing it

PROCEDURE:
  1. For each document d (index i):
       For each unique term t in d.tokens:
           index[t].push_back(i)
  2. For each term in index:
       Sort its postings list
  3. Return index

TIME COMPLEXITY:  O(sum U_d + sum df(t) log df(t))
SPACE COMPLEXITY: O(sum df(t)) = O(total postings)

---

ALGORITHM: QueryInvertedIndex(query_terms, index, docs) -> vector<int>
---------------------------------------------------
INPUT:  Query terms (stemmed), inverted index
OUTPUT: Candidate document IDs (sorted intersection)

PROCEDURE:
  1. For each query term t:
       postings[t] <- index[t]  (empty if t not in index)
  2. Result <- intersection of all postings lists
       (using sorted-list merge)
  3. Return Result

TIME COMPLEXITY:  O(sum df(t) for t in query)
WORST CASE:       O(N) if all query terms appear in all documents
BEST CASE:        O(1) if terms are very rare
```

### 3.6 Cosine Similarity (Sparse)

```
ALGORITHM: CosineSimilarity(v1, v2: map<string,double>) -> double
---------------------------------------------------
INPUT:  Two sparse TF-IDF vectors (term -> weight)
OUTPUT: Cosine similarity in [0, 1]

PROCEDURE:
  1. dot <- 0, norm1 <- 0, norm2 <- 0
  2. For each (term, w) in v1:
       norm1 += w^2
       If term in v2: dot += w * v2[term]
  3. For each (term, w) in v2:
       norm2 += w^2
  4. If norm1 == 0 or norm2 == 0: return 0
  5. Return dot / (sqrt(norm1) * sqrt(norm2))

TIME COMPLEXITY:  O(|v1| + |v2|)
OPTIMISED:        O(min(|v1|, |v2|)) with precomputed norms
```

### 3.7 Min-Heap Top-K Selection

```
ALGORITHM: TopK(scores: vector<pair<double,int>>, k: int) -> vector<pair<double,int>>
---------------------------------------------------
INPUT:  N (score, id) pairs
OUTPUT: K highest-scoring pairs, sorted descending

PROCEDURE:
  1. Initialise min-heap H of size k
  2. For each (score, id) in scores:
       If |H| < k:
           Push (score, id) to H
       Else if score > H.top().score:
           Pop H.top()
           Push (score, id) to H
  3. Extract all from H into result vector
  4. Reverse result (descending order)
  5. Return result

TIME COMPLEXITY:  O(N log K)
SPACE COMPLEXITY: O(K)
COMPARISON:       Full sort is O(N log N); heap wins when K << N
```

### 3.8 KMP String Search

```
ALGORITHM: BuildLPS(pattern: string) -> vector<int>
---------------------------------------------------
INPUT:  Search pattern
OUTPUT: Longest Proper Prefix which is also Suffix array

PROCEDURE:
  1. lps[0] <- 0; len <- 0; i <- 1
  2. While i < |pattern|:
       If pattern[i] == pattern[len]:
           len++; lps[i] <- len; i++
       Else if len != 0:
           len <- lps[len - 1]
       Else:
           lps[i] <- 0; i++
  3. Return lps

TIME COMPLEXITY:  O(m)  where m = |pattern|
SPACE COMPLEXITY: O(m)

---

ALGORITHM: KMPSearch(text, pattern) -> bool
---------------------------------------------------
PROCEDURE:
  1. lps <- BuildLPS(pattern)
  2. i <- 0, j <- 0
  3. While i < |text|:
       If tolower(text[i]) == tolower(pattern[j]):
           i++; j++
           If j == |pattern|: return true
       Else if j != 0:
           j <- lps[j-1]
       Else:
           i++
  4. Return false

TIME COMPLEXITY:  O(n + m)  where n = |text|, m = |pattern|
COMPARISON:       Naive search is O(n * m) in worst case
```

### 3.9 Levenshtein Distance (DP)

```
ALGORITHM: Levenshtein(a, b: string) -> int
---------------------------------------------------
INPUT:  Two strings
OUTPUT: Minimum edit distance (insertions + deletions + substitutions)

PROCEDURE:
  1. Create DP table of size (n+1) x (m+1)
  2. Base cases: dp[i][0] = i, dp[0][j] = j
  3. For i = 1 to n, j = 1 to m:
       cost <- (tolower(a[i-1]) == tolower(b[j-1])) ? 0 : 1
       dp[i][j] <- min(dp[i-1][j] + 1, dp[i][j-1] + 1, dp[i-1][j-1] + cost)
  4. Return dp[n][m]

TIME COMPLEXITY:  O(n * m)
SPACE COMPLEXITY: O(n * m)  [can optimise to O(min(n,m)) with rolling array]

---

ALGORITHM: FuzzySkillScore(skill1, skill2) -> double
---------------------------------------------------
PROCEDURE:
  1. dist <- Levenshtein(skill1, skill2)
  2. max_len <- max(|skill1|, |skill2|)
  3. Return 1.0 - (dist / max_len)

OUTPUT in [0.0, 1.0]  where 1.0 = exact match
```

### 3.10 Combined Scoring Function

```
ALGORITHM: CombinedScore(resume, job, alpha, beta) -> double
---------------------------------------------------
PARAMETERS: alpha = 0.7 (cosine weight), beta = 0.3 (skill match weight)
            Tunable via experiment

PROCEDURE:
  1. cosine <- CosineSimilarity(resume.tfidf_vector, job.tfidf_vector)
  2. skill_score <- AverageFuzzySkillScore(resume.skills, job.skills)
     Where for each resume skill r and job skill j:
       best_match(r) = max over j of FuzzySkillScore(r, j)
       skill_score = mean of best_match for all resume skills
  3. combined <- alpha * cosine + beta * skill_score
  4. Return combined

TIME COMPLEXITY:  O(|v_resume| + |v_job| + |resume_skills| * |job_skills| * avg_skill_len^2)
```

---

## 4. API Specification

### 4.1 Endpoints

#### POST /api/rank

**Purpose:** Rank jobs against a resume

**Request:**

```json
{
  "resume_text": "I am a software engineer with 5 years of experience in C++...",
  "top_k": 10
}
```

**Response:** See section 2.3 Ranking Result

**Status Codes:**

| Code | Meaning                              |
|------|--------------------------------------|
| 200  | Success, results returned            |
| 400  | Invalid request (empty resume_text)  |
| 500  | Internal error                       |

---

#### POST /api/upload

**Purpose:** Upload PDF resume, extract text

**Request:** `multipart/form-data` with field `file` (PDF)

**Response:**

```json
{
  "status": "success",
  "extracted_text": "John Doe\nSoftware Engineer\n..."
}
```

---

#### GET /api/jobs

**Purpose:** List job postings

**Query Parameters:**

| Param    | Type   | Default | Description        |
|----------|--------|---------|--------------------|
| page     | int    | 1       | Page number        |
| per_page | int    | 20      | Items per page     |
| search   | string | ""      | Filter by keyword  |

**Response:**

```json
{
  "total": 10000,
  "page": 1,
  "per_page": 20,
  "jobs": [ "..." ]
}
```

---

#### GET /api/benchmark

**Purpose:** Run all benchmark experiments, return timing data

**Query Parameters:**

| Param          | Type   | Default | Description                    |
|----------------|--------|---------|--------------------------------|
| max_n          | int    | 10000   | Maximum N for scaling test     |
| iterations     | int    | 100     | Repetitions per measurement    |
| experiments    | string | "all"   | Comma-separated experiment IDs |

**Experiment IDs:**

| ID              | Description                              |
|-----------------|------------------------------------------|
| brute_vs_index  | Brute-force scan vs inverted index       |
| sort_vs_heap    | Full sort vs min-heap top-K              |
| naive_vs_kmp    | Naive search vs KMP phrase matching      |
| tfidf_vs_bm25   | TF-IDF cosine vs BM25 ranking            |
| levenshtein_cost| Edit distance runtime vs string length   |

**Response:** See section 2.4 Benchmark Result

---

#### GET /api/health

**Response:**

```json
{
  "status": "ok",
  "version": "1.0.0",
  "jobs_loaded": 10000,
  "index_size_mb": 4.2,
  "uptime_seconds": 3600
}
```

---

### 4.2 Error Response Format

```json
{
  "status": "error",
  "error_code": "EMPTY_RESUME",
  "message": "Resume text cannot be empty",
  "timestamp": "2025-01-15T10:30:00Z"
}
```

---

## 5. Frontend Specification

### 5.1 Pages / Views

| View              | Route        | Description                                    |
|-------------------|-------------|------------------------------------------------|
| Upload            | `/`         | Resume input area + rank button                |
| Results           | `/results`  | Ranked job cards + timing breakdown            |
| Benchmarks        | `/bench`    | Algorithm comparison charts + raw data table   |
| Job Browser       | `/jobs`     | Browse all jobs (paginated, searchable)        |
| About / Methodology| `/about`   | Algorithm explanations + complexity summaries  |

### 5.2 Component Specifications

#### ResumeUpload Component

```
Props:      onResults: function(json)
State:      resumeText: string, isLoading: bool, topK: number
Features:   - Textarea for paste (10 rows)
            - PDF file input (calls POST /api/upload first)
            - Dropdown for K value: [5, 10, 20, 50]
            - "Rank Jobs" button with loading spinner
            - Validation: disable button if text < 50 chars
```

#### RankedList Component

```
Props:      jobs: array, timings: object
Features:   - Cards sorted by rank
            - Each card shows:
                * Rank number (#1, #2, ...)
                * Job title, company, location
                * Combined score (percentage bar, color-coded)
                * Cosine score and skill match score (separate bars)
                * Matched terms as tags/chips
                * Matched skills as tags/chips
            - "Show Details" expandable section per card
            - Total processing time banner at top
```

#### BenchmarkDashboard Component

```
Props:      experiments: array
Features:   - One chart per experiment (line chart)
            - X-axis: N (number of jobs)
            - Y-axis: time (ms)
            - Multiple lines per chart (one per algorithm variant)
            - Toggle between line/bar chart
            - "Run Benchmarks" button (calls GET /api/benchmark)
            - Download CSV button
            - Table view toggle (raw numbers)
            - Annotated complexity labels on each chart
```

#### TimingChart Component (Reusable)

```
Props:      data: { labels: [], datasets: [] }, title: string
Library:    Chart.js (react-chartjs-2)
Features:   - Responsive line chart
            - Log-scale toggle
            - Tooltip shows exact values
            - Complexity annotation in subtitle
```

### 5.3 UI Style Guidelines

| Element       | Specification                                          |
|---------------|--------------------------------------------------------|
| Color scheme  | Dark background (#1a1a2e), accent (#e94560)           |
| Font          | Inter or JetBrains Mono                                 |
| Score bar     | Green (>70%), Yellow (40-70%), Red (<40%)              |
| Animations    | Fade-in on results, smooth bar fills (300ms)           |
| Responsive    | Mobile-friendly (single column on < 768px)             |
| Accessibility | Proper ARIA labels, keyboard navigable                 |

---

## 6. Project Structure

```
job-ranking-system/
|
|-- backend/
|   |-- CMakeLists.txt
|   |-- main.cpp                         # API server entry
|   |
|   |-- algorithms/
|   |   |-- tokenizer.h
|   |   |-- tokenizer.cpp
|   |   |-- stemmer.h                    # Porter Stemmer
|   |   |-- stemmer.cpp
|   |   |-- tfidf.h
|   |   |-- tfidf.cpp
|   |   |-- bm25.h
|   |   |-- bm25.cpp
|   |   |-- inverted_index.h
|   |   |-- inverted_index.cpp
|   |   |-- cosine.h
|   |   |-- cosine.cpp
|   |   |-- heap_topk.h
|   |   |-- heap_topk.cpp
|   |   |-- kmp.h
|   |   |-- kmp.cpp
|   |   |-- levenshtein.h
|   |   |-- levenshtein.cpp
|   |
|   |-- pipeline/
|   |   |-- ranking_pipeline.h
|   |   |-- ranking_pipeline.cpp
|   |   |-- scoring.h
|   |
|   |-- benchmark/
|   |   |-- benchmark_engine.h
|   |   |-- benchmark_engine.cpp
|   |   |-- experiment_brute_vs_index.cpp
|   |   |-- experiment_sort_vs_heap.cpp
|   |   |-- experiment_naive_vs_kmp.cpp
|   |   |-- experiment_tfidf_vs_bm25.cpp
|   |
|   |-- api/
|   |   |-- routes.h
|   |   |-- routes.cpp
|   |
|   |-- utils/
|   |   |-- timer.h
|   |   |-- timer.cpp
|   |   |-- json_io.h
|   |   |-- json_io.cpp
|   |   |-- pdf_extractor.h
|   |   |-- pdf_extractor.cpp
|   |
|   |-- third_party/
|   |   |-- httplib.h
|   |   |-- nlohmann/
|   |       |-- json.hpp
|   |
|   |-- data/
|       |-- jobs.json
|       |-- stopwords.txt
|       |-- skills_taxonomy.json
|       |-- sample_resumes/
|           |-- resume_backend_dev.txt
|           |-- resume_data_science.txt
|           |-- resume_frontend_dev.txt
|
|-- frontend/
|   |-- package.json
|   |-- vite.config.js
|   |-- index.html
|   |-- public/
|   |   |-- favicon.ico
|   |-- src/
|       |-- main.jsx
|       |-- App.jsx
|       |-- App.css
|       |-- api/
|       |   |-- client.js
|       |-- components/
|       |   |-- ResumeUpload.jsx
|       |   |-- ResumeUpload.css
|       |   |-- RankedList.jsx
|       |   |-- RankedList.css
|       |   |-- JobCard.jsx
|       |   |-- JobCard.css
|       |   |-- TimingChart.jsx
|       |   |-- TimingChart.css
|       |   |-- BenchmarkDashboard.jsx
|       |   |-- BenchmarkDashboard.css
|       |   |-- ScoreBar.jsx
|       |   |-- Navbar.jsx
|       |   |-- Navbar.css
|       |-- pages/
|       |   |-- UploadPage.jsx
|       |   |-- ResultsPage.jsx
|       |   |-- BenchmarkPage.jsx
|       |   |-- JobsPage.jsx
|       |   |-- AboutPage.jsx
|       |-- utils/
|           |-- formatters.js
|
|-- experiments/
|   |-- run_benchmarks.cpp
|   |-- run_quality_eval.cpp
|   |-- generate_plots.py
|
|-- evaluation/
|   |-- ground_truth.json
|   |-- precision_recall.py
|   |-- results/
|       |-- precision_at_5.csv
|       |-- recall_at_5.csv
|       |-- ndcg_at_5.csv
|
|-- docs/
|   |-- PRD.md
|   |-- SPEC.md
|   |-- COMPLEXITY_ANALYSIS.md
|   |-- REPORT.md / report.pdf
|
|-- tests/
|   |-- test_tokenizer.cpp
|   |-- test_stemmer.cpp
|   |-- test_tfidf.cpp
|   |-- test_inverted_index.cpp
|   |-- test_cosine.cpp
|   |-- test_heap_topk.cpp
|   |-- test_kmp.cpp
|   |-- test_levenshtein.cpp
|   |-- CMakeLists.txt
|
|-- .gitignore
|-- README.md
|-- Makefile
```

---

## 7. Build & Run Instructions

### 7.1 Prerequisites

| Tool        | Version          | Purpose                  |
|-------------|------------------|--------------------------|
| C++ compiler| GCC 10+ or Clang 12+ | Backend compilation |
| CMake       | 3.16+            | Build system             |
| Node.js     | 18+              | Frontend build           |
| npm / yarn  | 9+               | Package manager          |

### 7.2 Backend Build

```bash
cd backend
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

**Outputs:**
- `./server` -- REST API server
- `./run_benchmarks` -- CLI benchmark runner (from experiments/)
- `./run_tests` -- unit test suite

### 7.3 Backend Run

```bash
# Start API server (loads jobs.json, builds index at startup)
./server --port 8080 --data ../data/jobs.json

# Run all benchmarks (writes to ../results/)
./run_benchmarks --max-n 10000 --iterations 100 --output ../results/benchmarks.csv

# Run quality evaluation
./run_quality_eval --ground-truth ../../evaluation/ground_truth.json
```

### 7.4 Frontend Build & Run

```bash
cd frontend
npm install
npm run dev          # Development (http://localhost:3000)

# Production build
npm run build        # Outputs to frontend/dist/
npm run preview      # Preview production build
```

### 7.5 Makefile Shortcuts

```makefile
.PHONY: all backend frontend benchmark test clean

all: backend frontend

backend:
	cd backend && mkdir -p build && cd build && cmake .. -DCMAKE_BUILD_TYPE=Release && make -j4

frontend:
	cd frontend && npm install && npm run build

benchmark: backend
	./backend/build/run_benchmarks --max-n 10000 --iterations 100

test: backend
	./backend/build/run_tests

clean:
	rm -rf backend/build frontend/dist frontend/node_modules results/*
```

---

## 8. Testing Strategy

### 8.1 Unit Tests (C++)

| Module           | Test Cases                                                     |
|------------------|----------------------------------------------------------------|
| tokenizer        | Empty string, single word, punctuation-heavy, unicode, long text |
| stemmer          | Known Porter examples (consign->consign, agreed->agre, relational->relat) |
| tfidf            | Single doc, two docs, term not in corpus, empty doc             |
| inverted_index   | Build correctness, query with no match, query with all match    |
| cosine           | Identical vectors -> 1.0, orthogonal -> 0.0, empty vector -> 0.0 |
| heap_topk        | K=1, K=N, K>N, all equal scores, negative scores               |
| kmp              | Pattern at start/middle/end, overlapping patterns, empty pattern |
| levenshtein      | Identical -> 0, completely different, one-char difference, empty strings |

**Test framework:** Catch2 (header-only) or Google Test

**Coverage target:** >= 80% line coverage (use gcov/lcov)

### 8.2 Integration Tests

| Test                          | Method                                                    |
|-------------------------------|-----------------------------------------------------------|
| Full ranking pipeline         | Known resume -> known top-3 jobs (regression test)        |
| API response format           | Verify JSON schema matches spec                           |
| Index query matches brute     | Random 100 queries: inverted index result == brute result |
| Benchmark reproducibility     | Run twice, assert variance < 20%                          |

### 8.3 Performance Tests

| Test                           | Measurement                     | Acceptance           |
|--------------------------------|---------------------------------|----------------------|
| Rank 10K jobs                  | Total latency (ms)              | < 500 ms             |
| Index build time               | Startup duration (ms)           | < 2000 ms for 10K    |
| Memory usage                   | RSS after loading 50K jobs      | < 500 MB             |
| Concurrent requests (optional) | 10 sequential requests          | No crashes, < 500ms  |

---

## 9. Benchmark Specification

### 9.1 Experiment Definitions

#### Experiment 1: Brute Force vs Inverted Index

```
PURPOSE: Show that inverted index retrieval is faster than scanning all jobs

SETUP:
  - N = {100, 250, 500, 1000, 2500, 5000, 10000}
  - 100 iterations per N (report median)
  - Query: resume token set

METHOD (Brute Force):
  For each job j in all_jobs:
      Compute cosine(resume, j)
  Take top-K

METHOD (Inverted Index):
  candidate_ids <- QueryInvertedIndex(resume_tokens, index)
  For each candidate j in candidate_ids:
      Compute cosine(resume, j)
  Take top-K

EXPECTED RESULT:
  - Inverted index is faster for selective queries (few terms)
  - Speedup increases with N
  - Graph: two lines diverging as N grows

METRICS: wall_time_ms (median of 100 runs)
```

#### Experiment 2: Full Sort vs Min-Heap Top-K

```
PURPOSE: Show heap-based selection is O(N log K) vs O(N log N) for sort

SETUP:
  - N = {100, 500, 1000, 5000, 10000, 50000}
  - K = {5, 10, 20, 50}
  - 100 iterations per (N, K) pair

METHOD (Sort):
  Sort all N scores descending -> take first K

METHOD (Heap):
  Min-heap of size K -> iterate all N -> extract K

EXPECTED RESULT:
  - Heap faster when K << N
  - Gap widens as N increases and K stays small
  - For K=N, sort may be competitive (same complexity)

METRICS: wall_time_ms (median of 100 runs)
```

#### Experiment 3: Naive vs KMP String Search

```
PURPOSE: Show KMP's O(n+m) advantage over naive O(n*m)

SETUP:
  - Text sizes: {1KB, 10KB, 100KB, 1MB}
  - Pattern lengths: {5, 10, 20, 50 chars}
  - 1000 iterations per config

METHOD (Naive):
  For each position i in text:
      Compare text[i..i+m-1] with pattern
  -> Worst case: O(n * m)

METHOD (KMP):
  Build LPS array for pattern: O(m)
  Scan text with LPS-guided backtracking: O(n)
  -> Total: O(n + m)

EXPECTED RESULT:
  - KMP significantly faster for repetitive text
  - Speedup increases with text size and pattern length
  - For short patterns, gap is smaller

METRICS: wall_time_ms (median of 1000 runs)
```

#### Experiment 4: TF-IDF Cosine vs BM25

```
PURPOSE: Compare ranking quality and runtime of two IR scoring methods

SETUP:
  - Same dataset, same queries (20 ground-truth resumes)
  - 100 iterations for timing

METHOD (TF-IDF + Cosine):
  Standard TF-IDF vectors, cosine similarity

METHOD (BM25):
  BM25 formula with k1=1.2, b=0.75

EXPECTED RESULT:
  - BM25 slightly better quality (NDCG@5) on long documents
  - TF-IDF cosine slightly faster
  - Quality difference is the key finding

METRICS: wall_time_ms, NDCG@5, Precision@5
```

#### Experiment 5: Levenshtein Cost Scaling

```
PURPOSE: Visualise O(n * m) DP cost vs string lengths

SETUP:
  - String lengths: {5, 10, 20, 50, 100, 200}
  - Random English-like strings
  - 10000 iterations per config

EXPECTED RESULT:
  - Quadratic growth in time as strings get longer
  - Matches theoretical O(n * m) bound

METRICS: wall_time_ms (median)
```

### 9.2 Benchmark Output Format

**CSV (benchmarks.csv):**

```csv
experiment,N,K,algorithm,time_ms_median,time_ms_p25,time_ms_p75
brute_vs_index,1000,10,brute_force,45.3,43.1,47.8
brute_vs_index,1000,10,inverted_index,12.7,11.9,13.5
sort_vs_heap,5000,10,full_sort,28.4,27.2,29.1
sort_vs_heap,5000,10,min_heap,3.1,2.9,3.3
```

**JSON (API response):** See section 2.4

### 9.3 Statistical Rigor

| Parameter            | Value                                          |
|----------------------|------------------------------------------------|
| Iterations per run   | 100 (timing), 1000 (string search)            |
| Reported statistic   | Median (robust to outliers)                   |
| Variance reporting   | Report IQR (p25-p75) as error bars            |
| Warm-up              | 10 iterations discarded before measurement    |
| CPU pinning          | Optional: `taskset -c 0 ./run_benchmarks`     |
| Compiler optimisation| `-O2` flag for all benchmarks                 |

---

## 10. Quality Evaluation

### 10.1 Ground Truth

- 20 resume-job pairs, manually labelled
- Each resume: paste real job description + skills as "resume"
- Each has 5 "ideal" job IDs (positions 1-5 in the ideal ranking)

### 10.2 Metrics

```
Precision@K = |relevant docs in top K| / K

Recall@K = |relevant docs in top K| / |all relevant docs|

NDCG@K = DCG@K / IDCG@K

  Where:
    DCG@K  = SUM from i=1 to K of (2^rel_i - 1) / log2(i + 1)
    IDCG@K = DCG@K of the ideal ranking
    rel_i  = relevance grade of doc at position i
             (2 = ideal match, 1 = good match, 0 = not relevant)
```

### 10.3 Target Scores

| Metric     | Target  | Rationale                              |
|------------|---------|----------------------------------------|
| Precision@5| >= 0.60 | 3 out of top 5 are genuinely relevant  |
| Recall@5   | >= 0.50 | At least half of relevant jobs in top 5|
| NDCG@5     | >= 0.55 | Ranking order is reasonably correct    |

---

## 11. Dependencies

### 11.1 Backend (C++)

| Dependency      | Type              | Size      | Purpose                    |
|-----------------|-------------------|-----------|----------------------------|
| cpp-httplib     | Header-only       | ~200 KB   | HTTP server                |
| nlohmann/json   | Header-only       | ~700 KB   | JSON parsing/serialisation |
| Catch2          | Header-only       | ~500 KB   | Unit testing               |
| (optional) Poppler / pdfium | Library | System  | PDF text extraction        |

**No algorithmic dependencies.** All IR/search algorithms are from scratch.

### 11.2 Frontend (JavaScript)

| Dependency         | Version | Purpose               |
|--------------------|---------|-----------------------|
| react              | 18.x    | UI framework          |
| react-dom          | 18.x    | DOM rendering         |
| react-router-dom   | 6.x     | Client-side routing   |
| chart.js           | 4.x     | Benchmark charts      |
| react-chartjs-2    | 5.x     | Chart.js React wrapper|
| vite               | 5.x     | Build tool            |

### 11.3 Evaluation (Python -- optional)

| Dependency  | Purpose                           |
|-------------|-----------------------------------|
| matplotlib  | Generate report plots from CSV    |
| pandas      | Tabular metrics summary           |

---

## Appendix A: Complexity Summary Table

| Algorithm          | Time (Build)      | Time (Query)        | Space         | Comparison       |
|--------------------|-------------------|---------------------|---------------|------------------|
| Tokenizer          | O(L)              | --                  | O(T)          | --               |
| Porter Stemmer     | O(W)              | --                  | O(W)          | --               |
| TF-IDF Build       | O(total_tokens)   | --                  | O(V)          | --               |
| BM25 Score         | --                | O(|q|) per doc      | O(1)          | vs TF-IDF cosine |
| Inverted Index     | O(sum U_d)        | O(sum df(t))        | O(sum df(t))  | vs O(N) brute    |
| Cosine Similarity  | --                | O(min(|v1|,|v2|))   | O(1)          | --               |
| Min-Heap Top-K     | --                | O(N log K)          | O(K)          | vs O(N log N)    |
| KMP Search         | O(m) LPS          | O(n + m)            | O(m)          | vs O(n*m) naive  |
| Levenshtein        | --                | O(n * m)            | O(n * m)      | DP baseline      |

## Appendix B: API Status Codes

| Code | Meaning                          | When                                  |
|------|----------------------------------|---------------------------------------|
| 200  | OK                               | Successful request                    |
| 201  | Created                          | (Reserved for future)                 |
| 400  | Bad Request                      | Missing/invalid fields                |
| 404  | Not Found                        | Job ID not found                      |
| 413  | Payload Too Large                | Resume > 10MB                         |
| 422  | Unprocessable Entity             | Valid JSON but semantically invalid   |
| 500  | Internal Server Error            | Unexpected crash                      |
| 503  | Service Unavailable              | Index not yet built                   |