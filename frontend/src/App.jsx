import { useEffect, useState } from 'react';
import ResumeUpload from './components/ResumeUpload.jsx';
import RankedList from './components/RankedList.jsx';
import BenchmarkDashboard from './components/BenchmarkDashboard.jsx';

const IconRank = () => (
  <svg width="15" height="15" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round" aria-hidden="true">
    <path d="M4 20V10M12 20V4M20 20v-6" />
  </svg>
);

const IconChart = () => (
  <svg width="15" height="15" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round" aria-hidden="true">
    <path d="M3 3v18h18" />
    <path d="m7 14 4-4 3 3 5-6" />
  </svg>
);

// Two-tab app (V1): Rank + Benchmarks. No router — view is plain state.
export default function App() {
  const [tab, setTab] = useState('rank');
  const [rankResponse, setRankResponse] = useState(null);
  const [health, setHealth] = useState(null);

  useEffect(() => {
    fetch('/api/health')
      .then((r) => r.json())
      .then(setHealth)
      .catch(() => setHealth({ status: 'offline' }));
  }, []);

  const online = health && health.status === 'ok';

  return (
    <div className="app">
      <header className="navbar">
        <div className="brand">
          <span className="brand-mark" aria-hidden="true">
            <svg width="16" height="16" viewBox="0 0 24 24" fill="currentColor">
              <path d="M13 2 4 14h6l-1 8 9-12h-6l1-8z" />
            </svg>
          </span>
          <span className="brand-text">
            CV Sharp
            <span className="brand-sub">Find your perfect match</span>
          </span>
        </div>

        <nav className="tabs" role="tablist" aria-label="Views">
          <button
            role="tab"
            aria-selected={tab === 'rank'}
            className={tab === 'rank' ? 'tab active' : 'tab'}
            onClick={() => setTab('rank')}
          >
            <IconRank /> Rank
          </button>
          <button
            role="tab"
            aria-selected={tab === 'bench'}
            className={tab === 'bench' ? 'tab active' : 'tab'}
            onClick={() => setTab('bench')}
          >
            <IconChart /> Benchmarks
          </button>
        </nav>

        <div
          className={online ? 'status online' : 'status offline'}
          role="status"
        >
          <span className="dot" aria-hidden="true" />
          {online
            ? `API online · ${health.jobs_loaded.toLocaleString()} jobs`
            : 'API offline'}
        </div>
      </header>

      <main className="content">
        {tab === 'rank' ? (
          <>
            <ResumeUpload
              onResults={(json) => setRankResponse(json)}
              hasResults={!!rankResponse}
              jobsLoaded={online ? health.jobs_loaded : null}
            />
            {rankResponse && <RankedList response={rankResponse} />}
          </>
        ) : (
          <BenchmarkDashboard />
        )}
      </main>

      <footer className="footer">
        <span>
          Analysis of Algorithms — Mini Project · algorithms from scratch in
          C++17: tokenizer · Porter stemmer · TF-IDF · inverted index · cosine ·
          min-heap top-K
        </span>
      </footer>
    </div>
  );
}
