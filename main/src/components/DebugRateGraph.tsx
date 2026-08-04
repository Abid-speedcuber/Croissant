import { useEffect, useRef } from "react";

export type RatePoint = { t: number; sol: number; node: number };

export type LiveDebugData = {
  now: number;
  running: boolean;
  startTime: number;
  stopTime: number;
  history: RatePoint[];
  totalSolutions: number;
  totalNodes: number;
};

const MAX_DRAW_POINTS = 600;
const COL_SOL = "#4a90d9";
const COL_NODE = "#e0a24a";
const COL_TEXT = "#aeb9cd";
const COL_GRID = "rgba(122, 143, 173, 0.16)";

const formatAxis = (value: number) => {
  if (value >= 1e6) return `${(value / 1e6).toFixed(1)}m`;
  if (value >= 1e3) return `${(value / 1e3).toFixed(1)}k`;
  if (value >= 100) return `${Math.round(value)}`;
  return value.toFixed(value < 10 ? 1 : 0);
};

export function DebugRateGraph({
  getLive,
  solLabel,
  nodeLabel,
  avgLabel,
  stddevLabel,
}: {
  getLive: () => LiveDebugData;
  solLabel: string;
  nodeLabel: string;
  avgLabel: string;
  stddevLabel: string;
}) {
  const canvasRef = useRef<HTMLCanvasElement | null>(null);

  useEffect(() => {
    const canvas = canvasRef.current;
    if (!canvas) return;
    const ctx = canvas.getContext("2d");
    if (!ctx) return;

    let raf = 0;
    const draw = () => {
      raf = requestAnimationFrame(draw);
      const data = getLive();
      const refTime = data.running ? data.now : (data.stopTime || data.now);
      const elapsedSec = data.startTime > 0 ? (refTime - data.startTime) / 1000 : 0;

      const dpr = window.devicePixelRatio || 1;
      const cssW = canvas.clientWidth;
      const cssH = canvas.clientHeight;
      if (cssW <= 0 || cssH <= 0) return;
      const pxW = Math.round(cssW * dpr);
      const pxH = Math.round(cssH * dpr);
      if (canvas.width !== pxW || canvas.height !== pxH) {
        canvas.width = pxW;
        canvas.height = pxH;
      }
      ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
      ctx.clearRect(0, 0, cssW, cssH);

      const padL = 46;
      const padR = 50;
      const padT = 40;
      const padB = 6;
      const plotW = cssW - padL - padR;
      const plotH = cssH - padT - padB;
      if (plotW <= 0 || plotH <= 0) return;

      const series = data.history;
      const avgSol = elapsedSec > 0 ? data.totalSolutions * 60 / elapsedSec : 0;
      const avgNode = elapsedSec > 0 ? data.totalNodes / elapsedSec : 0;

      let solMax = 1;
      let nodeMax = 1;
      for (const s of series) {
        if (s.sol > solMax) solMax = s.sol;
        if (s.node > nodeMax) nodeMax = s.node;
      }
      solMax = Math.max(solMax, avgSol);
      nodeMax = Math.max(nodeMax, avgNode);
      solMax *= 1.15;
      nodeMax *= 1.15;

      const tSpan = Math.max(1, elapsedSec);
      const x = (t: number) => padL + (t / tSpan) * plotW;
      const ySol = (v: number) => padT + plotH - (v / solMax) * plotH;
      const yNode = (v: number) => padT + plotH - (v / nodeMax) * plotH;

      ctx.font = "10px system-ui";
      ctx.lineWidth = 1;
      ctx.strokeStyle = COL_GRID;
      ctx.textAlign = "left";
      ctx.textBaseline = "middle";
      ctx.fillStyle = COL_TEXT;
      for (let i = 0; i <= 4; i++) {
        const gy = Math.round(padT + (plotH / 4) * i) + 0.5;
        ctx.beginPath();
        ctx.moveTo(padL, gy);
        ctx.lineTo(cssW - padR, gy);
        ctx.stroke();
        ctx.fillText(formatAxis(solMax * (1 - i / 4)), 2, gy - 0.5);
        ctx.fillText(formatAxis(nodeMax * (1 - i / 4)), cssW - padR + 4, gy - 0.5);
      }

      const drawLine = (series: RatePoint[], field: "sol" | "node", yFn: (v: number) => number, color: string) => {
        if (series.length < 2) return;
        ctx.strokeStyle = color;
        ctx.lineWidth = 1.6;
        ctx.lineJoin = "round";
        ctx.beginPath();
        const step = Math.max(1, Math.ceil(series.length / MAX_DRAW_POINTS));
        let started = false;
        for (let i = 0; i < series.length; i += step) {
          const px = x(series[i].t);
          const py = yFn(series[i][field]);
          if (!started) { ctx.moveTo(px, py); started = true; }
          else ctx.lineTo(px, py);
        }
        ctx.stroke();
      };
      const drawAvg = (avg: number, yFn: (v: number) => number, color: string) => {
        if (avg <= 0) return;
        ctx.save();
        ctx.strokeStyle = color;
        ctx.lineWidth = 1;
        ctx.setLineDash([5, 4]);
        ctx.beginPath();
        ctx.moveTo(padL, yFn(avg));
        ctx.lineTo(cssW - padR, yFn(avg));
        ctx.stroke();
        ctx.restore();
      };

      drawLine(series, "sol", ySol, COL_SOL);
      drawLine(series, "node", yNode, COL_NODE);
      drawAvg(avgSol, ySol, COL_SOL);
      drawAvg(avgNode, yNode, COL_NODE);

      const sigma = (field: "sol" | "node", avg: number) => {
        // Skip the leading zero-rate samples (the first couple of seconds before
        // the solver finds anything); including them inflates the stddev.
        let start = 0;
        while (start < series.length && series[start][field] === 0) start++;
        const n = series.length - start;
        if (n < 2) return 0;
        let sq = 0;
        for (let i = start; i < series.length; i++) sq += (series[i][field] - avg) ** 2;
        return Math.sqrt(sq / n);
      };
      const sigmaSol = sigma("sol", avgSol);
      const sigmaNode = sigma("node", avgNode);

      ctx.font = "11px system-ui";
      ctx.textBaseline = "top";
      ctx.textAlign = "left";
      const legendRows: { color: string; label: string; rest: string }[] = [
        { color: COL_SOL, label: solLabel, rest: `${avgLabel} ${avgSol.toFixed(1)}/min  ${stddevLabel} ${sigmaSol.toFixed(1)}/min` },
        { color: COL_NODE, label: nodeLabel, rest: `${avgLabel} ${formatAxis(avgNode)}/s  ${stddevLabel} ${formatAxis(sigmaNode)}/s` },
      ];
      let ly = 5;
      for (const row of legendRows) {
        ctx.fillStyle = row.color;
        ctx.fillRect(padL, ly + 4, 10, 3);
        let lx = padL + 14;
        ctx.fillStyle = COL_TEXT;
        ctx.fillText(row.label, lx, ly);
        lx += ctx.measureText(row.label).width + 14;
        ctx.fillStyle = COL_TEXT;
        ctx.fillText(row.rest, lx, ly);
        ly += 16;
      }
    };
    raf = requestAnimationFrame(draw);
    return () => cancelAnimationFrame(raf);
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, []);

  return <canvas ref={canvasRef} className="debug-rate-graph" />;
}
