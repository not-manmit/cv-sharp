import { useState } from 'react';
import { rankJobs } from '../api/client.js';

const SAMPLE_RESUME = `John Doe - Software Engineer
Email: john.doe@example.com

Senior software engineer with 6 years of experience in C++ and Python.
Built REST APIs with PostgreSQL, deployed services with Docker on Linux.
Experience with distributed systems, cloud infrastructure and CI/CD.
Led a team migrating monoliths to microservices on AWS.`;

const TOP_K_OPTIONS = [5, 10, 20, 50];

const PIPELINE = [
  'Tokenize',
  'Porter Stem',
  'TF-IDF',
  'Inverted Index',
  'Cosine + Skill',
  'Min-Heap Top-K',
];

// SPEC §5.2 — ResumeUpload component.
export default function ResumeUpload({ onResults, hasResults, jobsLoaded }) {
  const [text, setText] = useState(SAMPLE_RESUME);
  const [topK, setTopK] = useState(10);
  const [loading, setLoading] = useState(false);
  const [error, setError] = useState('');

  const tooShort = text.trim().length < 50;
  const chars = text.trim().length;

  async function handleSubmit(e) {
    e.preventDefault();
    if (tooShort || loading) return;
    setLoading(true);
    setError('');
    try {
      const json = await rankJobs(text, topK);
      onResults(json);
    } catch (err) {
      setError(err.message);
    } finally {
      setLoading(false);
    }
  }

  return (
    <form className="card upload-card" onSubmit={handleSubmit}>
      <div className="hero">
        <span className="eyebrow">RESUME → RANKED JOBS</span>
        <h1>
          Find where your resume{' '}
          <span className="grad-text">actually fits</span>
        </h1>
        <p className="hero-sub">
          Paste your resume and the engine ranks{' '}
          {jobsLoaded ? jobsLoaded.toLocaleString() : '2,000'} job postings in
          milliseconds — no ML libraries, just classical information retrieval.
        </p>
        <div className="pipeline" aria-label="Processing pipeline">
          {PIPELINE.map((step, i) => (
            <span className="pipeline-group" key={step}>
              <span className="pipeline-chip">{step}</span>
              {i < PIPELINE.length - 1 && (
                <span className="pipeline-arrow" aria-hidden="true">
                  ›
                </span>
              )}
            </span>
          ))}
        </div>
      </div>

      <label className="field-label" htmlFor="resume-input">
        Resume text
      </label>
      <textarea
        id="resume-input"
        rows={10}
        value={text}
        onChange={(e) => setText(e.target.value)}
        placeholder="Paste your resume text here (at least 50 characters)…"
        spellCheck={false}
      />

      <div className="upload-controls">
        <label className="k-select">
          Top K
          <select
            aria-label="Top K results"
            value={topK}
            onChange={(e) => setTopK(Number(e.target.value))}
          >
            {TOP_K_OPTIONS.map((k) => (
              <option key={k} value={k}>
                {k}
              </option>
            ))}
          </select>
        </label>
        <span className={chars >= 50 ? 'char-count' : 'char-count short'}>
          {chars} / 50 min characters
        </span>
        <button
          type="submit"
          className="primary"
          disabled={tooShort || loading}
          aria-busy={loading}
        >
          {loading ? (
            <>
              <span className="spinner" aria-hidden="true" /> Ranking…
            </>
          ) : (
            <>
              <svg
                width="15"
                height="15"
                viewBox="0 0 24 24"
                fill="currentColor"
                aria-hidden="true"
              >
                <path d="M13 2 4 14h6l-1 8 9-12h-6l1-8z" />
              </svg>
              Rank Jobs
            </>
          )}
        </button>
      </div>

      {error && (
        <div className="error" role="alert">
          {error}
        </div>
      )}
      {hasResults && (
        <div className="hint subtle">New results replace the list below.</div>
      )}
    </form>
  );
}
