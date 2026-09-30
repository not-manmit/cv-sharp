import { useState } from 'react';
import {
  Chart as ChartJS,
  CategoryScale,
  LinearScale,
  PointElement,
  LineElement,
  BarElement,
  Title,
  Tooltip,
  Legend,
  LogarithmicScale,
} from 'chart.js';
import { Chart } from 'react-chartjs-2';

ChartJS.register(
  CategoryScale,
  LinearScale,
  LogarithmicScale,
  PointElement,
  LineElement,
  BarElement,
  Title,
  Tooltip,
  Legend
);

const PALETTE = ['#e94560', '#34e2e8', '#fbbf24', '#7c5cff'];

// Reusable chart (SPEC §5.2 TimingChart): line/bar + log-scale toggles.
export default function TimingChart({ title, subtitle, labels, datasets, type }) {
  const [logScale, setLogScale] = useState(false);
  const data = {
    labels,
    datasets: datasets.map((d, i) => ({
      label: d.label,
      data: d.data,
      borderColor: PALETTE[i % PALETTE.length],
      backgroundColor: PALETTE[i % PALETTE.length] + '99',
      tension: 0.25,
      fill: false,
    })),
  };
  const options = {
    responsive: true,
    maintainAspectRatio: false,
    plugins: {
      legend: { labels: { color: '#e9edf8' } },
      title: { display: true, text: title, color: '#ffffff', font: { size: 15 } },
      subtitle: {
        display: !!subtitle,
        text: subtitle,
        color: '#94a3c4',
        font: { size: 11, style: 'italic' },
      },
      tooltip: { callbacks: { label: (c) => `${c.dataset.label}: ${c.parsed.y} ms` } },
    },
    scales: {
      x: {
        title: { display: true, text: 'N (jobs)', color: '#94a3c4' },
        ticks: { color: '#94a3c4' },
        grid: { color: 'rgba(148,163,184,0.08)' },
      },
      y: {
        type: logScale ? 'logarithmic' : 'linear',
        title: { display: true, text: 'time (ms)', color: '#94a3c4' },
        ticks: { color: '#94a3c4' },
        grid: { color: 'rgba(148,163,184,0.08)' },
      },
    },
  };
  return (
    <div className="chart-box">
      <div className="chart-controls">
        <button className="chip-btn" onClick={() => setLogScale(!logScale)}>
          {logScale ? 'Log scale: on' : 'Log scale: off'}
        </button>
      </div>
      <div className="chart-canvas">
        <Chart type={type} data={data} options={options} />
      </div>
    </div>
  );
}
