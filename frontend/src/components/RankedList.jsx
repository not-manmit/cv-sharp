import { useState } from 'react';

function toneOf(pct) {
  return pct > 70 ? 'green' : pct >= 40 ? 'yellow' : 'red';
}

// Circular combined-score gauge (SVG, no extra dependency).
function ScoreRing({ value }) {
  const pct = Math.round(value * 100);
  const tone = toneOf(pct);
  const r = 27;
  const circ = 2 * Math.PI * r;
  return (
    <div className={`ring ring-${tone}`} aria-label={`Match ${pct}%`}>
      <svg viewBox="0 0 64 64" width="72" height="72" role="img">
        <circle
          className="ring-track"
          cx="32"
          cy="32"
          r={r}
          fill="none"
          strokeWidth="6"
        />
        <circle
          className="ring-fill"
          cx="32"
          cy="32"
          r={r}
          fill="none"
          strokeWidth="6"
          strokeLinecap="round"
          strokeDasharray={`${(circ * pct) / 100} ${circ}`}
          transform="rotate(-90 32 32)"
        />
        <text className="ring-value" x="32" y="31" textAnchor="middle">
          {pct}%
        </text>
        <text className="ring-label" x="32" y="44" textAnchor="middle">
          MATCH
        </text>
      </svg>
    </div>
  );
}

function MiniBar({ label, value }) {
  const pct = Math.round(value * 100);
  const tone = toneOf(pct);
  return (
    <div className="mini-bar">
      <div className="mini-bar-head">
        <span>{label}</span>
        <span className="mini-bar-val">{pct}%</span>
      </div>
      <div className="bar-track">
        <div
          className={`bar-fill ${tone}`}
          style={{ width: `${pct}%` }}
          role="progressbar"
          aria-valuenow={pct}
          aria-valuemin={0}
          aria-valuemax={100}
          aria-label={`${label} ${pct}%`}
        />
      </div>
    </div>
  );
}

function Stat({ label, value, accent }) {
  return (
    <div className={accent ? 'stat accent' : 'stat'}>
      <div className="stat-v">{value}</div>
      <div className="stat-l">{label}</div>
    </div>
  );
}

const IconBuilding = () => (
  <svg width="12" height="12" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round" aria-hidden="true">
    <rect x="4" y="3" width="16" height="18" rx="2" />
    <path d="M9 7h2M13 7h2M9 11h2M13 11h2M9 15h2M13 15h2" />
  </svg>
);

const IconPin = () => (
  <svg width="12" height="12" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round" aria-hidden="true">
    <path d="M12 21s-7-6.1-7-11a7 7 0 1 1 14 0c0 4.9-7 11-7 11z" />
    <circle cx="12" cy="10" r="2.5" />
  </svg>
);

const IconChevron = ({ open }) => (
  <svg
    width="13"
    height="13"
    viewBox="0 0 24 24"
    fill="none"
    stroke="currentColor"
    strokeWidth="2.4"
    strokeLinecap="round"
    strokeLinejoin="round"
    aria-hidden="true"
    style={{ transform: open ? 'rotate(180deg)' : 'none', transition: 'transform .2s' }}
  >
    <path d="m6 9 6 6 6-6" />
  </svg>
);

function JobCard({ job, index }) {
  const [open, setOpen] = useState(false);
  const medal = ['', 'gold', 'silver', 'bronze'][job.rank] || '';
  return (
    <article
      className={`job-card ${medal}`}
      style={{ '--d': `${Math.min(index, 8) * 55}ms` }}
    >
      <div className="rank-badge" aria-label={`Rank ${job.rank}`}>
        <span className="rank-hash">#</span>
        {job.rank}
      </div>

      <div className="job-body">
        <div className="job-head">
          <div className="job-title-block">
            <h3>{job.title}</h3>
            <div className="job-meta">
              <span className="meta-item">
                <IconBuilding /> {job.company}
              </span>
              <span className="meta-dot" aria-hidden="true">
                ·
              </span>
              <span className="meta-item">
                <IconPin /> {job.location}
              </span>
            </div>
          </div>
          <ScoreRing value={job.combined_score} />
        </div>

        <div className="score-grid">
          <MiniBar label="Cosine" value={job.cosine_score} />
          <MiniBar label="Skill match" value={job.skill_match_score ?? 0} />
        </div>

        {job.matched_terms?.length > 0 && (
          <div className="chips">
            <span className="chips-label">matched</span>
            {job.matched_terms.map((t) => (
              <span className="chip" key={t}>
                {t}
              </span>
            ))}
          </div>
        )}

        <div className="job-actions">
          <button
            className="link-btn"
            onClick={() => setOpen(!open)}
            aria-expanded={open}
          >
            {open ? 'Hide details' : 'Show details'} <IconChevron open={open} />
          </button>
          <span className="job-id">{job.job_id}</span>
        </div>

        {open && <p className="job-desc fade-in">{job.description}</p>}
      </div>
    </article>
  );
}

// SPEC §5.2 — RankedList component (stat row + ranked cards).
export default function RankedList({ response }) {
  const { results, timings, total_jobs_considered, candidates_after_filter } =
    response;
  return (
    <section className="results" aria-label="Ranked results">
      <div className="stats-row fade-in">
        <Stat label="Total time" value={`${timings.total_ms} ms`} accent />
        <Stat
          label="Jobs scanned"
          value={total_jobs_considered.toLocaleString()}
        />
        <Stat label="Candidates" value={candidates_after_filter.toLocaleString()} />
        <Stat label="Index filter" value={`${timings.candidate_filter_ms} ms`} />
        <Stat label="Scoring" value={`${timings.scoring_ms} ms`} />
        <Stat label="Top-K" value={`${timings.topk_select_ms} ms`} />
      </div>

      {results.length === 0 && (
        <div className="error" role="alert">
          No candidate jobs matched your resume terms.
        </div>
      )}

      {results.map((job, i) => (
        <JobCard key={job.job_id + job.rank} job={job} index={i} />
      ))}
    </section>
  );
}
