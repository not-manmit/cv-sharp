# Guide — How to Run the Job Ranking Engine

Everything in this project runs on **Windows PowerShell**. All commands are run
from the project root (the folder that contains `backend/`, `frontend/`, `docs/`).

---

## 1. Prerequisites

| Tool | Needed for | Check with |
|---|---|---|
| CMake ≥ 3.16 | building the C++ backend | `cmake --version` |
| g++ (C++17) or MSVC | compiling the backend | `g++ --version` |
| Ninja (optional) | faster builds | `ninja --version` |
| Node.js 18+ | building/running the frontend | `node --version` |
| Python 3.x | regenerating the dataset (optional) | `python --version` |

> **New terminal and tools not found?** Refresh your PATH first:
> ```powershell
> $env:Path = [System.Environment]::GetEnvironmentVariable("Path","Machine") + ";" + [System.Environment]::GetEnvironmentVariable("Path","User")
> ```

---

## 2. First-time build

### 2.1 Backend (C++)

```powershell
cmake -S backend -B backend/build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build backend/build
```

- Omit `-G Ninja` if you don't have Ninja (CMake will use the default generator).
- Result: `backend\build\server.exe` and `backend\build\run_benchmarks.exe`.

### 2.2 Frontend (React)

```powershell
cd frontend
npm install        # first time only
npm run build      # produces frontend\dist
cd ..
```

> The C++ server serves `frontend\dist` directly, so **always rebuild the
> frontend after changing any React code**.

---

## 3. Run the server (normal mode — recommended)

```powershell
.\backend\build\server.exe --port 8080
```

Then open **http://localhost:8080** in your browser.

What you should see in the terminal:

```
[server] loaded 2000 jobs from backend/data/jobs.json (index terms: ...)
[server] serving frontend from frontend/dist
```

### Server flags

| Flag | Default | Meaning |
|---|---|---|
| `--port 8080` | 8080 | TCP port to listen on |
| `--data <jobs.json>` | auto-detected | path to the job dataset |
| `--stopwords <file>` | auto-detected | stop-word list |
| `--frontend <dist_dir>` | auto-detected (`frontend/dist`) | static UI folder |
| `--help` | — | print usage |

Auto-detection works when you run from the project root; use the flags if you
run from a different working directory.

---

## 4. Stop the server

| Situation | Command |
|---|---|
| Server in the foreground (normal case) | Press **Ctrl + C** in its terminal |
| Server running in background / hung | `Get-Process server -ErrorAction SilentlyContinue \| Stop-Process -Force` |
| Alternative | `taskkill /IM server.exe /F` |
| Verify it is gone | `Get-NetTCPConnection -LocalPort 8080 -ErrorAction SilentlyContinue` (should return nothing) |

### ⚠️ Important: stop the server BEFORE rebuilding the backend

Windows locks `server.exe` while it is running, so `cmake --build` will fail
with *"unable to open ... server.exe"* if the server is still up.

```powershell
# correct order
Get-Process server -ErrorAction SilentlyContinue | Stop-Process -Force
cmake --build backend/build
.\backend\build\server.exe --port 8080
```

---

## 5. Development mode (frontend hot-reload)

Use this while editing React code — the page reloads instantly on save.

**Terminal 1 — backend:**
```powershell
.\backend\build\server.exe --port 8080
```

**Terminal 2 — frontend:**
```powershell
cd frontend
npm run dev
```

Open **http://localhost:3000**. Vite serves the UI and proxies all `/api/*`
requests to the C++ backend on `:8080` (see `frontend/vite.config.js`).
Both terminals must stay open.

---

## 6. API quick reference

All endpoints can be tested with `curl` or PowerShell:

```powershell
# health check
Invoke-RestMethod http://localhost:8080/api/health

# rank a resume
$body = @{
  resume_text = "Senior software engineer with experience in C++ Python Docker and AWS."
  top_k = 10
} | ConvertTo-Json
Invoke-RestMethod -Uri http://localhost:8080/api/rank -Method Post -Body $body -ContentType "application/json"

# browse jobs (pagination + search)
Invoke-RestMethod "http://localhost:8080/api/jobs?page=1&per_page=20&search=devops"

# run benchmarks live
Invoke-RestMethod "http://localhost:8080/api/benchmark?max_n=2000&iterations=30"
```

| Method | Path | Body / Query |
|---|---|---|
| POST | `/api/rank` | `{"resume_text": "...", "top_k": 10}` |
| GET | `/api/jobs` | `?page=1&per_page=20&search=...` |
| GET | `/api/benchmark` | `?max_n=2000&iterations=30` |
| GET | `/api/health` | — |

Errors come back as HTTP 400 with
`{"status":"error","error_code":"...","message":"..."}`.

---

## 7. Benchmarks (CLI)

```powershell
.\backend\build\run_benchmarks.exe --max-n 2000 --iterations 100 --output results\benchmarks.csv
```

- Runs **brute force vs inverted index** and **full sort vs min-heap top-K**.
- Prints median timings and speedups; also writes `results\benchmarks.csv`.
- The same experiments are available in the UI under the **Benchmarks** tab
  (line/bar charts, log scale, table view, CSV download).

---

## 8. Regenerate the dataset (optional)

```powershell
python tools/generate_jobs.py --count 2000
```

- Seeded (seed = 42) → fully reproducible, same output every run.
- Writes `backend\data\jobs.json`. **Restart the server** afterwards so it
  reloads the data.

---

## 9. Typical full workflow (copy-paste)

```powershell
# 0. one-time PATH refresh in a fresh terminal
$env:Path = [System.Environment]::GetEnvironmentVariable("Path","Machine") + ";" + [System.Environment]::GetEnvironmentVariable("Path","User")

# 1. build backend (first time configures it)
cmake -S backend -B backend/build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build backend/build

# 2. build frontend
cd frontend; npm install; npm run build; cd ..

# 3. run
.\backend\build\server.exe --port 8080
#  → open http://localhost:8080

# 4. stop with Ctrl+C, then restart whenever you rebuilt anything
```

---

## 10. Troubleshooting

| Symptom | Fix |
|---|---|
| `error: jobs.json not found` | Run from the project root, or pass `--data backend/data/jobs.json` |
| Build fails: cannot open `server.exe` | Stop the server first (section 4) |
| UI shows old code after editing React | Rebuild: `cd frontend; npm run build` |
| Blank page / 404 on `/` | `frontend\dist` missing → run `npm run build` |
| Port already in use | `.\backend\build\server.exe --port 8081`, or free port 8080 (section 4) |
| `cmake` / `g++` not found | Refresh PATH (section 1) |
| Dev mode: API calls fail (CORS/network) | Backend on `:8080` must be running; UI must be opened at `:3000`, not `:8080` |
| Changed `jobs.json`, but rankings unchanged | Restart the server (data is loaded once at startup) |
