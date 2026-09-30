// ---------------------------------------------------------------------------
// run_benchmarks.cpp — CLI benchmark runner (SPEC §7.3, §9)
//
//   ./run_benchmarks [--data backend/data/jobs.json] [--max-n 2000]
//                    [--iterations 100] [--output results/benchmarks.csv]
//
// Runs both V1 experiments and writes the CSV from SPEC §9.2 plus a short
// speedup summary on stdout.
// ---------------------------------------------------------------------------
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <string>
#include <vector>

#include "../backend/pipeline.h"
#include "../backend/benchmark.h"

namespace {

std::string first_existing(const std::vector<std::string>& candidates) {
    for (const std::string& p : candidates) {
        std::ifstream f(p);
        if (f.good()) return p;
    }
    return "";
}

}  // namespace

int main(int argc, char** argv) {
    int max_n = 2000;
    int iterations = 100;
    std::string data_path, output_path = "results/benchmarks.csv";

    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        auto next = [&]() -> std::string { return i + 1 < argc ? argv[++i] : ""; };
        if (a == "--data") data_path = next();
        else if (a == "--max-n") max_n = std::atoi(next().c_str());
        else if (a == "--iterations") iterations = std::atoi(next().c_str());
        else if (a == "--output") output_path = next();
        else if (a == "--help" || a == "-h") {
            std::printf(
                "Usage: run_benchmarks [--data jobs.json] [--max-n 2000] "
                "[--iterations 100] [--output results/benchmarks.csv]\n");
            return 0;
        }
    }
    if (max_n < 100) max_n = 100;
    if (iterations < 5) iterations = 5;

    if (data_path.empty())
        data_path = first_existing({"backend/data/jobs.json", "data/jobs.json",
                                    "../backend/data/jobs.json", "../data/jobs.json"});
    if (data_path.empty()) {
        std::fprintf(stderr, "error: jobs.json not found (use --data)\n");
        return 1;
    }

    jre::RankingEngine engine;
    std::string err;
    if (!engine.load_jobs(data_path, err)) {
        std::fprintf(stderr, "error: %s\n", err.c_str());
        return 1;
    }
    engine.build();
    std::printf("[bench] corpus: %zu jobs\n", engine.docs().size());

    // Query = first posting's text (real corpus term distribution).
    const auto& docs = engine.docs();
    jre::Document q;
    q.tokens = engine.tokenize_stem(docs[0].title + " " + docs[0].description);
    engine.model().embed(q);

    std::printf("[bench] experiment 1/2: brute_force vs inverted_index "
                "(max_n=%d, K=10, iters=%d)\n", max_n, iterations);
    jre::bench::ExperimentResult e1 = jre::bench::exp_brute_vs_index(
        docs, q, q.tokens, max_n, 10, iterations);

    std::printf("[bench] experiment 2/2: full_sort vs min_heap "
                "(max_n=%d, K=10, iters=%d)\n", max_n, iterations);
    jre::bench::ExperimentResult e2 = jre::bench::exp_sort_vs_heap(max_n, 10, iterations);

    // Write CSV (SPEC §9.2)
    {
        size_t slash = output_path.find_last_of("/\\");
        if (slash != std::string::npos) {
            std::string dir = output_path.substr(0, slash);
#ifdef _WIN32
            std::string cmd = "if not exist \"" + dir + "\" mkdir \"" + dir + "\"";
#else
            std::string cmd = "mkdir -p \"" + dir + "\"";
#endif
            std::system(cmd.c_str());
        }
        std::ofstream out(output_path, std::ios::binary);
        if (!out) {
            std::fprintf(stderr, "error: cannot write %s\n", output_path.c_str());
            return 1;
        }
        out << jre::bench::to_csv({e1, e2});
        std::printf("[bench] wrote %s\n", output_path.c_str());
    }

    // Speedup summary
    auto ratio_last = [](const jre::bench::ExperimentResult& e) {
        size_t i = e.ns.size() - 1;
        double a = e.median[0][i], b = e.median[1][i];
        return b > 0 ? a / b : 0.0;
    };
    auto ratio_all = [](const jre::bench::ExperimentResult& e) {
        double prod = 1.0;
        for (size_t i = 0; i < e.ns.size(); ++i) {
            double a = e.median[0][i], b = e.median[1][i];
            prod *= (b > 0 ? a / b : 1.0);
        }
        return e.ns.empty() ? 1.0 : std::pow(prod, 1.0 / static_cast<double>(e.ns.size()));
    };

    std::printf("\n=== Summary (median ms) ===\n");
    std::printf("%-18s %8s %8s %10s %8s %10s\n", "experiment", "N", "K",
                "algo_A", "algo_B", "speedup");
    for (const jre::bench::ExperimentResult* e : {&e1, &e2}) {
        for (size_t i = 0; i < e->ns.size(); ++i) {
            double a = e->median[0][i], b = e->median[1][i];
            std::printf("%-18s %8d %8d %10.3f %8.3f %9.2fx  %s / %s\n",
                        e->name.c_str(), e->ns[i], e->ks[i], a, b,
                        b > 0 ? a / b : 0.0, e->series[0].c_str(),
                        e->series[1].c_str());
        }
        std::printf("%-18s geometric-mean speedup over all N: %.2fx\n\n",
                    e->name.c_str(), ratio_all(*e));
        (void)ratio_last;
    }

    const size_t last = e1.ns.size() - 1;
    double bf = e1.median[0][last], ix = e1.median[1][last];
    double sh = e2.median[0][last], hp = e2.median[1][last];
    std::printf("NFR check @ N=%d (K=10):\n", e1.ns[last]);
    std::printf("  NFR-02 inverted-index speedup  : %.2fx (target >= 3x)  [%s]\n",
                ix > 0 ? bf / ix : 0.0, (ix > 0 && bf / ix >= 3.0) ? "PASS" : "CHECK");
    std::printf("  NFR-03 heap top-K speedup      : %.2fx (target >= 1.5x) [%s]\n",
                hp > 0 ? sh / hp : 0.0, (hp > 0 && sh / hp >= 1.5) ? "PASS" : "CHECK");
    return 0;
}
