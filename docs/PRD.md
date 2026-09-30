# Product Requirements Document
# Job Ranking Engine — Resume-to-Job Matching System

| Field          | Value                                    |
|----------------|------------------------------------------|
| Project Name   | Job Ranking Engine                       |
| Version        | 1.0                                      |
| Author         | [Your Name]                              |
| Course         | Analysis of Algorithms (Mini Project)    |
| Date           | [Today's Date]                           |
| Status         | Draft → Approved                         |

---

## 1. Problem Statement

Job seekers face information overload. A single job board may have thousands of
postings. Manually scanning each posting to evaluate fit against one's resume is
time-consuming, subjective, and inconsistent. There exists a need for a system
that algorithmically compares a resume against a corpus of job postings and
returns a ranked list of the most suitable roles — while demonstrating that
the underlying algorithms are efficient and well-analysed.

## 2. Goals & Objectives

### 2.1 Primary Goals

| ID  | Goal                                              | Success Criteria                                      |
|-----|---------------------------------------------------|-------------------------------------------------------|
| G1  | Accept a resume (text or PDF) and rank jobs        | System returns top-K ranked jobs in < 500 ms for 1K jobs |
| G2  | Implement all core algorithms from scratch in C++  | Zero dependency on ML/IR libraries (no sklearn, no Lucene) |
| G3  | Provide empirical complexity analysis              | Benchmark data plotted against theoretical bounds     |
| G4  | Compare algorithm variants side-by-side            | At least 3 meaningful comparisons (see §4.4)          |
| G5  | Deliver an interactive UI for demonstration        | React frontend with upload, results, and chart views   |

### 2.2 Secondary Goals

| ID  | Goal                                                    |
|-----|---------------------------------------------------------|
| G6  | Support fuzzy skill matching via edit distance          |
| G7  | Support multi-keyword phrase matching via KMP search    |
| G8  | Generate CSV benchmark data for report inclusion        |
| G9  | Provide a clean REST API for potential reuse            |

## 3. Target Users

| Persona            | Description                                             |
|--------------------|---------------------------------------------------------|
| Student/Job Seeker | Uploads resume, wants ranked job recommendations        |
| Professor/Evaluator| Reviews system to assess algorithm understanding & depth |
| Developer          | Extends algorithm modules or adds new data sources       |

## 4. Functional Requirements

### 4.1 Resume Ingestion

| ID    | Requirement                                                            | Priority |
|-------|------------------------------------------------------------------------|----------|
| FR-01 | User can paste resume text into a text area                            | Must     |
| FR-02 | User can upload a PDF file; system extracts text via pdfplumber/extraction layer | Should   |
| FR-03 | System normalises text: lowercases, strips punctuation, removes stop words | Must     |
| FR-04 | System tokenises text into word tokens (length > 2)                    | Must     |
| FR-05 | System applies stemming (Porter Stemmer) to each token                 | Must     |

### 4.2 Job Data Ingestion

| ID    | Requirement                                                            | Priority |
|-------|------------------------------------------------------------------------|----------|
| FR-06 | System loads job postings from a local JSON dataset (1K–10K jobs)      | Must     |
| FR-07 | Each job record contains: title, description, company, location, skills| Must     |
| FR-08 | (Optional) System can fetch jobs from an external API (Adzuna/USAJOBS) | Could    |
| FR-09 | Job postings are normalised and tokenised identically to resumes       | Must     |

### 4.3 Feature Extraction & Indexing

| ID    | Requirement                                                            | Priority |
|-------|------------------------------------------------------------------------|----------|
| FR-10 | System computes TF-IDF vectors for every job posting and the resume    | Must     |
| FR-11 | System builds an inverted index (term → postings list)                 | Must     |
| FR-12 | Index is built once at startup and reused for all queries              | Must     |
| FR-13 | System supports BM25 scoring as an alternative to TF-IDF cosine        | Should   |

### 4.4 Ranking & Retrieval

| ID    | Requirement                                                            | Priority |
|-------|------------------------------------------------------------------------|----------|
| FR-14 | System computes cosine similarity between resume vector and each job   | Must     |
| FR-15 | System selects top-K results using a min-heap (default K = 10)         | Must     |
| FR-16 | User can specify K (top-5, top-10, top-20)                             | Must     |
| FR-17 | System matches resume skills against job skills using KMP string search | Should   |
| FR-18 | System supports fuzzy skill matching using Levenshtein distance        | Should   |
| FR-19 | System computes a combined score: α·cosine + β·skill_match + γ·phrase  | Should   |

### 4.5 Algorithm Comparison & Benchmarking

| ID    | Requirement                                                            | Priority |
|-------|------------------------------------------------------------------------|----------|
| FR-20 | System can run brute-force scan vs. inverted index retrieval           | Must     |
| FR-21 | System can run full sort vs. min-heap for top-K selection              | Must     |
| FR-22 | System can run naive string search vs. KMP for phrase matching         | Should   |
| FR-23 | System can run TF-IDF cosine vs. BM25 for ranking quality              | Should   |
| FR-24 | System exports timing results to CSV (N, algorithm, time_ms)           | Must     |
| FR-25 | Frontend renders timing comparison as line/bar charts (Chart.js)       | Must     |

### 4.6 User Interface

| ID    | Requirement                                                            | Priority |
|-------|------------------------------------------------------------------------|----------|
| FR-26 | React frontend with three views: Upload, Results, Benchmarks           | Must     |
| FR-27 | Results view shows ranked job cards with match percentage bar          | Must     |
| FR-28 | Results view shows matched terms between resume and each job           | Should   |
| FR-29 | Benchmarks view shows runtime vs. N charts per comparison              | Must     |
| FR-30 | UI shows total processing time for each ranking request                | Must     |

### 4.7 REST API

| ID    | Requirement                                                            | Priority |
|-------|------------------------------------------------------------------------|----------|
| FR-31 | `POST /api/rank` — accepts resume text + K, returns ranked jobs JSON   | Must     |
| FR-32 | `GET  /api/jobs` — returns all job postings (paginated)                | Must     |
| FR-33 | `POST /api/upload` — accepts PDF file, returns extracted text          | Should   |
| FR-34 | `GET  /api/benchmark` — runs comparison suite, returns timing data     | Must     |
| FR-35 | `GET  /api/health` — returns server status                             | Must     |

## 5. Non-Functional Requirements

| ID     | Category       | Requirement                                                    |
|--------|----------------|----------------------------------------------------------------|
| NFR-01 | Performance    | Rank 10K jobs in < 500 ms (single-core, typical laptop)        |
| NFR-02 | Performance    | Inverted index query must be ≥ 3× faster than brute force at N=10K |
| NFR-03 | Performance    | Min-heap top-K must be ≥ 1.5× faster than full sort at N=10K, K=10 |
| NFR-04 | Scalability    | Handle datasets up to 50K job postings without OOM            |
| NFR-05 | Accuracy       | Precision@5 ≥ 0.6 on 20-sample ground-truth set               |
| NFR-06 | Accuracy       | Recall@5 ≥ 0.5 on 20-sample ground-truth set                  |
| NFR-07 | Portability    | Compiles on Linux, macOS, Windows (C++17, CMake)               |
| NFR-08 | Usability      | Frontend loads in < 2 s; no build step required for demo       |
| NFR-09 | Maintainability| Each algorithm is a separate compilation unit with header-only interface |
| NFR-10 | Testability    | Unit tests for every algorithm module (≥ 80% coverage)         |
| NFR-11 | Documentation  | Every function has a doc comment with time/space complexity     |

## 6. Success Metrics

| Metric                          | Target                        | Measurement Method          |
|---------------------------------|-------------------------------|-----------------------------|
| Rank quality (Precision@5)      | ≥ 0.60                        | Manual label comparison     |
| Rank quality (NDCG@5)           | ≥ 0.55                        | Manual label comparison     |
| Inverted index speedup          | ≥ 3× over brute force         | Benchmark CSV               |
| Heap top-K speedup              | ≥ 1.5× over full sort         | Benchmark CSV               |
| KMP speedup over naive search   | ≥ 2× on 100KB text            | Benchmark CSV               |
| All experiments reproducible    | Yes — single command          | `./run_benchmarks`          |
| Report includes complexity proofs| Yes — per algorithm           | Written report              |

## 7. Out of Scope (v1)

| Item                                      | Reason                                  |
|-------------------------------------------|-----------------------------------------|
| Neural embeddings (BERT, Word2Vec, etc.)  | Black box; defeats complexity analysis  |
| Real-time job board scraping at scale     | Rate limits, fragility, not algorithmic |
| User authentication / accounts            | Not relevant to the course              |
| Database (PostgreSQL, etc.)               | JSON files are sufficient for demo      |
| Production deployment (AWS, Docker)       | Out of scope for mini project           |
| Mobile app                                | Web UI is sufficient                    |
| Multi-language resume support             | English only for v1                     |
| Fine-tuned ML ranking model               | Would obscure algorithmic core          |

## 8. Constraints & Assumptions

| #  | Constraint / Assumption                                                   |
|----|---------------------------------------------------------------------------|
| 1  | All core algorithms are implemented in C++17 from scratch (no IR libraries)|
| 2  | Frontend uses React (Vite); backend uses cpp-httplib or Crow              |
| 3  | Job dataset is a static JSON file (Kaggle or hand-curated)                |
| 4  | PDF text extraction may use a library (pdfium, poppler) — plumbing only   |
| 5  | Evaluation dataset (ground truth) consists of 20 manually labelled pairs  |
| 6  | Single-user system (no concurrency handling required in v1)               |
| 7  | UTF-8 text; English stopwords list hardcoded                              |

## 9. Milestones & Timeline

| Week | Milestone                                             | Deliverable                          |
|------|-------------------------------------------------------|--------------------------------------|
| 1    | Core algorithm modules complete                       | tokenizer, stemmer, TF-IDF, cosine, heap — all unit-tested |
| 1    | Dataset loaded and normalised                         | jobs.json → processed documents      |
| 2    | REST API + React UI working end-to-end                | Upload resume → see ranked jobs      |
| 2    | Inverted index + KMP + Levenshtein integrated         | Fuzzy/phrase matching in results     |
| 3    | Benchmark experiments executed + charts generated     | CSV + Chart.js visualisations        |
| 3    | Quality evaluation on ground-truth set                | Precision@5, Recall@5, NDCG@5       |
| 3    | Final report (complexity proofs + results + analysis) | PDF report submitted                 |

## 10. Risks

| Risk                                        | Likelihood | Impact | Mitigation                                   |
|---------------------------------------------|------------|--------|-----------------------------------------------|
| Job dataset too small / low quality          | Medium     | High   | Use Kaggle dataset + augment manually         |
| C++ build issues across OS                  | Medium     | Medium | Use CMake + standard C++17; test on all OS    |
| PDF extraction produces garbled text        | Medium     | Medium | Offer text-paste fallback always              |
| Timing benchmarks too noisy                 | Medium     | Medium | Run 100 iterations, report median             |
| Scope creep (wanting ML models)             | Low        | High   | Strictly follow §7 out-of-scope               |