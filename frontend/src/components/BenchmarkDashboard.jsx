import { useState } from 'react';
import { runBenchmarks, experimentsToCsv } from '../api/client.js';
import TimingChart from './TimingChart.jsx';

const COMPLEXITY = {
  brute_vs_index: 'brute force O(N)  vs  inverted index O(Σ df(t))',
  sort_vs_heap: 'full sort O(N log N)  vs  min-heap O(N log K)',
};

const KPI_LABEL = {
  brute_vs_index: 'Inverted index speedup',
  sort_vs_heap: 'Min-heap speedup',
};

const IconRun = () => (
  <svg width="14" height="14" viewBox="0 0 24 24" fill="currentColor" aria-hidden="true">
    <path d="M8 5v14l11-7z" />
  </svg>
);
const IconTable = () => (
  <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" aria-hidden="true">
    <rect x="3" y="4" width="18" height="16" rx="2" />
    <path d="M3 10h18M9 10v10" />
  </svg>
);
const IconDownload = () => (
  <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round" aria-hidden="true">
    <path d="M12 3v12m0 0 4-4m-4 4-4-4M4 21h16" />
  </svg>
);
const IconSwap = () => (
  <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round" aria-hidden="true">
    <path d="M7 3v14m0 0-3-3m3 3 3-3M17 21V7m0 0 3 3m-3-3-3 3" />
  </svg>
);

function seriesFor(exp) {
  const algorithms = Object.keys(exp.data[0] || {}).filter((k) =>
    k.endsWith('_ms')
  );
  return {
    algorithms,
    labels: exp.data.map((row) => String(row.N)),
    datasets: algorithms.map((algo) => ({
      label: algo.replace('_ms', ''),
      data: exp.data.map((row) => row[algo]),
    })),
  };
}

function download(filename, content, mime) {
  const blob = new Blob([content], { type: mime });
  const url = URL.createObjectURL(blob);
  const a = document.createElement('a');
  a.href = url;
  a.download = filename;
  a.click();
  URL.revokeObjectURL(url);
}

function KpiCard({ exp }) {
  const { datasets, labels } = seriesFor(exp);
  const last = exp.data.length - 1;
  const a = datasets[0]?.data[last];
  const b = datasets[1]?.data[last];
  const ratio = b > 0 ? a / b : null;
  return (
    <div className="kpi fade-in">
      <div className="kpi-label">{KPI_LABEL[exp.name] || exp.name}</div>
      <div className="kpi-big grad-text">
        {ratio ? `${ratio.toFixed(1)}×` : '—'}
      </div>
      <div className="kpi-sub">
        {datasets[0]?.label} / {datasets[1]?.label} @ N ={' '}
        {Number(labels[last]).toLocaleString()} · K = 10
      </div>
    </div>
  );
}

// SPEC §5.2 — BenchmarkDashboard component.
export default function BenchmarkDashboard() {
  const [experiments, setExperiments] = useState(null);
  const [loading, setLoading] = useState(false);
  const [error, setError] = useState('');
  const [chartType, setChartType] = useState('line');
  const [showTable, setShowTable] = useState(false);

  async function handleRun() {
    setLoading(true);
    setError('');
    try {
      const json = await runBenchmarks(2000, 30);
      setExperiments(json.experiments);
    } catch (err) {
      setError(err.message);
    } finally {
      setLoading(false);
    }
  }

  return (
    <section className="bench" aria-label="Benchmarks">
      <div className="card">
        <div className="bench-head">
          <div>
            <span className="eyebrow">EMPIRICAL COMPLEXITY</span>
            <h2>Algorithm comparison</h2>
            <p className="hint">
              Median of 30 timed runs after 10 warm-up iterations · single core
              · -O2 · N ∈ {'{100, 250, 500, 1000, 2000}'}, K = 10
            </p>
          </div>
          <button className="primary" onClick={handleRun} disabled={loading}>
            {loading ? (
              <>
                <span className="spinner" aria-hidden="true" /> Running…
              </>
            ) : (
              <>
                <IconRun /> Run Benchmarks
              </>
            )}
          </button>
        </div>

        {experiments && (
          <div className="bench-controls">
            <button
              className="chip-btn"
              onClick={() => setChartType(chartType === 'line' ? 'bar' : 'line')}
            >
              <IconSwap /> {chartType === 'line' ? 'Bar chart' : 'Line chart'}
            </button>
            <button className="chip-btn" onClick={() => setShowTable(!showTable)}>
              <IconTable /> {showTable ? 'Hide table' : 'Table view'}
            </button>
            <button
              className="chip-btn"
              onClick={() =>
                download(
                  'benchmarks.csv',
                  experimentsToCsv(experiments),
                  'text/csv'
                )
              }
            >
              <IconDownload /> Download CSV
            </button>
          </div>
        )}
        {error && (
          <div className="error" role="alert">
            {error}
          </div>
        )}
      </div>

      {experiments && (
        <div className="kpi-grid">
          {experiments.map((exp) => (
            <KpiCard exp={exp} key={`kpi-${exp.name}`} />
          ))}
        </div>
      )}

      {experiments?.map((exp) => {
        const { labels, datasets } = seriesFor(exp);
        return (
          <div className="card fade-in" key={exp.name}>
            <div className="chart-head">
              <div>
                <h3>{exp.name.replaceAll('_', ' vs ')}</h3>
                <p className="hint">{exp.description}</p>
              </div>
              <code className="complexity-tag">{COMPLEXITY[exp.name] || ''}</code>
            </div>
            <TimingChart
              title={exp.name}
              subtitle={COMPLEXITY[exp.name] || ''}
              labels={labels}
              datasets={datasets}
              type={chartType}
            />
            {showTable && (
              <table className="data-table">
                <thead>
                  <tr>
                    <th>N</th>
                    {datasets.map((d) => (
                      <th key={d.label}>{d.label} (ms)</th>
                    ))}
                    <th>speedup</th>
                  </tr>
                </thead>
                <tbody>
                  {labels.map((n, i) => {
                    const a = datasets[0].data[i];
                    const b = datasets[1]?.data[i];
                    return (
                      <tr key={n}>
                        <td>{Number(n).toLocaleString()}</td>
                        {datasets.map((d) => (
                          <td key={d.label}>{d.data[i].toFixed(3)}</td>
                        ))}
                        <td className="speedup-cell">
                          {b > 0 ? `${(a / b).toFixed(2)}×` : '—'}
                        </td>
                      </tr>
                    );
                  })}
                </tbody>
              </table>
            )}
          </div>
        );
      })}
    </section>
  );
}
