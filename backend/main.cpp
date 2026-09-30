// ---------------------------------------------------------------------------
// main.cpp — REST API server (SPEC §4), cpp-httplib
//
//   POST /api/rank      rank jobs against a resume (FR-31)
//   GET  /api/jobs      paginated job listing  (FR-32)
//   GET  /api/benchmark run comparison experiments (FR-34)
//   GET  /api/health    server status (FR-35)
//
// Also serves the built frontend (frontend/dist) when present, so the whole
// demo runs from a single process.
// ---------------------------------------------------------------------------
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <fstream>
#include <string>
#include <thread>
#include <vector>

#include "benchmark.h"
#include "pipeline.h"
#include "third_party/httplib.h"

using jre::RankingEngine;
using nlohmann::json;

namespace {

RankingEngine g_engine;
std::chrono::steady_clock::time_point g_start;
std::string g_version = "1.0.0";
double g_index_mb = 0.0;

std::string iso_now() {
    std::time_t t = std::time(nullptr);
    std::tm tm{};
#ifdef _WIN32
    gmtime_s(&tm, &t);
#else
    gmtime_r(&t, &tm);
#endif
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", &tm);
    return buf;
}

void set_cors(const httplib::Request&, httplib::Response& res) {
    res.set_header("Access-Control-Allow-Origin", "*");
    res.set_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    res.set_header("Access-Control-Allow-Headers", "Content-Type");
}

void error_response(httplib::Response& res, int status, const std::string& code,
                    const std::string& message) {
    json body;
    body["status"] = "error";
    body["error_code"] = code;
    body["message"] = message;
    body["timestamp"] = iso_now();
    res.status = status;
    res.set_content(body.dump(), "application/json");
}

// first existing candidate path, or empty
std::string first_existing(const std::vector<std::string>& candidates) {
    for (const std::string& p : candidates) {
        std::ifstream f(p);
        if (f.good()) return p;
    }
    return "";
}

}  // namespace

int main(int argc, char** argv) {
    int port = 8080;
    std::string data_path, stopwords_path, frontend_path;

    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        auto next = [&]() -> std::string { return i + 1 < argc ? argv[++i] : ""; };
        if (a == "--port") port = std::atoi(next().c_str());
        else if (a == "--data") data_path = next();
        else if (a == "--stopwords") stopwords_path = next();
        else if (a == "--frontend") frontend_path = next();
        else if (a == "--help" || a == "-h") {
            std::printf("Usage: server [--port 8080] [--data jobs.json] "
                        "[--stopwords stopwords.txt] [--frontend dist_dir]\n");
            return 0;
        }
    }

    // Resolve defaults relative to common working directories.
    if (data_path.empty())
        data_path = first_existing({"backend/data/jobs.json", "data/jobs.json",
                                    "../data/jobs.json"});
    if (stopwords_path.empty())
        stopwords_path = first_existing({"backend/data/stopwords.txt",
                                         "data/stopwords.txt",
                                         "../data/stopwords.txt"});
    if (frontend_path.empty()) {
        if (std::ifstream("frontend/dist/index.html").good())
            frontend_path = "frontend/dist";
        else if (std::ifstream("../frontend/dist/index.html").good())
            frontend_path = "../frontend/dist";
    }

    if (data_path.empty()) {
        std::fprintf(stderr, "error: jobs.json not found (use --data)\n");
        return 1;
    }

    if (!stopwords_path.empty()) g_engine.load_stopwords(stopwords_path);

    std::string err;
    if (!g_engine.load_jobs(data_path, err)) {
        std::fprintf(stderr, "error: %s\n", err.c_str());
        return 1;
    }
    g_engine.build();
    g_start = std::chrono::steady_clock::now();
    g_index_mb = (g_engine.index_term_count() * 40.0) / (1024.0 * 1024.0);

    std::printf("[server] loaded %zu jobs from %s (index terms: %zu)\n",
                g_engine.docs().size(), data_path.c_str(),
                g_engine.index_term_count());

    httplib::Server svr;
    svr.set_default_headers({{"Access-Control-Allow-Origin", "*"},
                             {"Access-Control-Allow-Methods", "GET, POST, OPTIONS"},
                             {"Access-Control-Allow-Headers", "Content-Type"}});

    // --- POST /api/rank -----------------------------------------------------
    svr.Post("/api/rank", [](const httplib::Request& req, httplib::Response& res) {
        set_cors(req, res);
        json body;
        try {
            body = json::parse(req.body.empty() ? "{}" : req.body);
        } catch (...) {
            error_response(res, 400, "INVALID_JSON", "Request body is not valid JSON");
            return;
        }
        std::string resume = body.value("resume_text", "");
        if (resume.empty()) {
            error_response(res, 400, "EMPTY_RESUME", "Resume text cannot be empty");
            return;
        }
        if (resume.size() > 200000) {
            error_response(res, 413, "PAYLOAD_TOO_LARGE",
                           "Resume text exceeds 200KB limit");
            return;
        }
        int top_k = body.value("top_k", 10);
        if (top_k < 1) top_k = 1;
        if (top_k > 100) top_k = 100;
        res.set_content(g_engine.rank_json(resume, top_k).dump(),
                        "application/json");
    });

    // --- GET /api/jobs ------------------------------------------------------
    svr.Get("/api/jobs", [](const httplib::Request& req, httplib::Response& res) {
        set_cors(req, res);
        int page = req.has_param("page") ? std::atoi(req.get_param_value("page").c_str()) : 1;
        int per_page =
            req.has_param("per_page") ? std::atoi(req.get_param_value("per_page").c_str()) : 20;
        std::string search = req.has_param("search") ? req.get_param_value("search") : "";
        if (page < 1) page = 1;
        if (per_page < 1) per_page = 20;
        if (per_page > 100) per_page = 100;
        for (char& c : search) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));

        std::vector<const jre::Document*> matched;
        matched.reserve(g_engine.docs().size());
        for (const jre::Document& d : g_engine.docs()) {
            if (!search.empty()) {
                std::string hay = d.title + " " + d.company + " " + d.location +
                                  " " + d.description;
                for (char& c : hay)
                    c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
                if (hay.find(search) == std::string::npos) continue;
            }
            matched.push_back(&d);
        }

        size_t start = static_cast<size_t>(page - 1) * static_cast<size_t>(per_page);
        json jobs = json::array();
        for (size_t i = start; i < matched.size() &&
                               i < start + static_cast<size_t>(per_page);
             ++i) {
            const jre::Document* d = matched[i];
            jobs.push_back({{"id", d->id},
                            {"title", d->title},
                            {"company", d->company},
                            {"location", d->location},
                            {"description", d->description},
                            {"skills", d->skills}});
        }
        json out;
        out["total"] = matched.size();
        out["page"] = page;
        out["per_page"] = per_page;
        out["jobs"] = jobs;
        res.set_content(out.dump(), "application/json");
    });

    // --- GET /api/benchmark -------------------------------------------------
    svr.Get("/api/benchmark", [](const httplib::Request& req, httplib::Response& res) {
        set_cors(req, res);
        int max_n = req.has_param("max_n") ? std::atoi(req.get_param_value("max_n").c_str())
                                           : 2000;
        int iterations = req.has_param("iterations")
                             ? std::atoi(req.get_param_value("iterations").c_str())
                             : 30;
        if (max_n < 100) max_n = 100;
        if (max_n > 10000) max_n = 10000;
        if (iterations < 5) iterations = 5;
        if (iterations > 100) iterations = 100;

        // Query = first job's text (real corpus distribution).
        const auto& docs = g_engine.docs();
        jre::Document q;
        q.tokens = g_engine.tokenize_stem(docs[0].title + " " + docs[0].description);
        g_engine.model().embed(q);

        std::vector<jre::bench::ExperimentResult> results;
        results.push_back(jre::bench::exp_brute_vs_index(
            docs, q, q.tokens, max_n, 10, iterations));
        results.push_back(jre::bench::exp_sort_vs_heap(max_n, 10, iterations));
        res.set_content(jre::bench::to_json(results).dump(), "application/json");
    });

    // --- GET /api/health ----------------------------------------------------
    svr.Get("/api/health", [](const httplib::Request& req, httplib::Response& res) {
        set_cors(req, res);
        double uptime = std::chrono::duration<double>(
                            std::chrono::steady_clock::now() - g_start)
                            .count();
        json out;
        out["status"] = "ok";
        out["version"] = g_version;
        out["jobs_loaded"] = g_engine.docs().size();
        out["index_size_mb"] =
            static_cast<double>(static_cast<int>(g_index_mb * 100) / 100.0);
        out["uptime_seconds"] = static_cast<long>(uptime);
        res.set_content(out.dump(), "application/json");
    });

    // CORS preflight for each API route
    for (const char* path : {"/api/rank", "/api/jobs", "/api/benchmark",
                             "/api/health"}) {
        svr.Options(path, [](const httplib::Request& req, httplib::Response& res) {
            set_cors(req, res);
            res.status = 204;
        });
    }

    // Serve the production frontend if it was built.
    if (!frontend_path.empty()) {
        if (svr.set_mount_point("/", frontend_path))
            std::printf("[server] serving frontend from %s\n", frontend_path.c_str());
    }

    std::printf("[server] listening on http://localhost:%d\n", port);
    svr.listen("0.0.0.0", port);
    return 0;
}
