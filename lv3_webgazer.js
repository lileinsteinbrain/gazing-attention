// ---------- Parameters ----------
const ALPHA = 0.05;   // accumulation rate
const BETA  = 0.02;   // decay rate
const JITTER_K = 0.02;
const HISTORY = 20;

let p = 0.0;                 // persistence state
let gazeHistory = [];
let lambda_m = 0.01;

// ---------- ROI ----------
function inROI(x, y) {
  const cx = window.innerWidth * 0.5;
  const cy = window.innerHeight * 0.5;
  const rx = 180, ry = 130; // ellipse radius
  const nx = (x - cx) / rx;
  const ny = (y - cy) / ry;
  return (nx*nx + ny*ny) <= 1.0;
}

// ---------- Jitter ----------
function jitterStd(hist) {
  if (hist.length < 2) return 0;
  let mx = 0, my = 0;
  hist.forEach(p => { mx += p.x; my += p.y; });
  mx /= hist.length; my /= hist.length;
  let v = 0;
  hist.forEach(p => {
    v += (p.x-mx)**2 + (p.y-my)**2;
  });
  return Math.sqrt(v / hist.length);
}

// ---------- WebGazer ----------
webgazer
  .setGazeListener((data) => {
    if (!data) return;

    const x = data.x;
    const y = data.y;

    // history
    gazeHistory.push({x,y});
    if (gazeHistory.length > HISTORY) gazeHistory.shift();

    // hit
    const hit = inROI(x,y) ? 1 : 0;

    // persistence
    p = Math.max(0, Math.min(1, p + ALPHA*hit - BETA));

    // stability
    const jitter = jitterStd(gazeHistory);
    const stability = Math.exp(-JITTER_K * jitter);

    // lambda_m
    lambda_m = Math.max(0, Math.min(1, 0.6*p + 0.4*stability));

    document.getElementById("hud").innerText =
      `λₘ: ${lambda_m.toFixed(3)}`;
  })
  .begin();

webgazer.showVideo(true);
webgazer.showPredictionPoints(true);