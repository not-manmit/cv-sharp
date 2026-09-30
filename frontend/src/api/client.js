// Thin fetch wrapper for the C++ REST API (SPEC §4)
async function request(path, options = {}) {
  const res = await fetch(path, options);
  let data = null;
  try {
    data = await res.json();
  } catch {
    /* non-JSON body */
  }
  if (!res.ok) {
    const msg = data && data.message ? data.message : `HTTP ${res.status}`;
    throw new Error(msg);
  }
  return data;
}

export function rankJobs(resumeText, topK) {
  return request('/api/rank', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ resume_text: resumeText, top_k: topK }),
  });
}

export function fetchHealth() {
  return request('/api/health');
}

export function runBenchmarks(maxN = 2000, iterations = 30) {
  return request(
    `/api/benchmark?max_n=${maxN}&iterations=${iterations}`
  );
}

// Build the SPEC §9.2 CSV client-side from the API JSON (Download button).
export function experimentsToCsv(experiments) {
  let csv = 'experiment,N,K,algorithm,time_ms_median\n';
  for (const exp of experiments) {
    const algorithms = Object.keys(exp.data[0] || {}).filter((k) =>
      k.endsWith('_ms')
    );
    for (const row of exp.data) {
      for (const algo of algorithms) {
        csv += `${exp.name},${row.N},${row.K ?? 10},${algo.replace(
          '_ms',
          ''
        )},${row[algo]}\n`;
      }
    }
  }
  return csv;
}
