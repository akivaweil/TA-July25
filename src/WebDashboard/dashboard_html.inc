// Auto-served single-page dashboard for the central multi-machine view.
// Hosted by TA at GET "/". Vanilla HTML/CSS/JS, phone-friendly. Themed after
// the "Tuesday" web app (shadcn zinc dark): flat gray-900 canvas, navy
// chrome bar with the sky hairline, frosted glass-surface cards, white→blue
// gradient tool heading + 4px accent pill, blue gradient CTA, emerald/amber/
// red status tints, Geist-style font stack — one card-grid panel per machine
// behind a tab strip; default tab = Stage 1. Each panel is driven entirely by
// that machine's /api/status + /api/config (same REST contract for all four).
// Stage 1's panel also carries the 3 Inch / Minis mode toggle (reserved
// "__mode__" POST key), matching its old dashboard. Config sections are
// collapsible (a group whose first field reports collapsed:true starts closed,
// e.g. the Motors speed/accel block); fields flagged fromSteps are shown and
// edited in inches/sec on the front end while the POST still sends steps
// (using the machine's stepsPerInch), so the firmware contract is unchanged.
extern const char DASHBOARD_HTML[] PROGMEM = R"rawliteral(<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0, viewport-fit=cover">
<title>Production Line Dashboard</title>
<link rel="icon" href="data:image/svg+xml,<svg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 100 100'><text y='.9em' font-size='90'>⚙️</text></svg>">
<!-- Geist-style system font stack — rendered natively, no external font fetch -->
<meta name="theme-color" content="#0d183b">
<style>
  :root {
    /* "Tuesday" theme — shadcn zinc dark tokens + Tuesday app chrome */
    --canvas: #111827;                        /* gray-900 — page body */
    --surface: hsl(240 10% 3.9%);             /* #09090b — inputs */
    --sidebar: hsl(225 64% 14%);              /* #0d183b — navy chrome */
    --text-main: hsl(0 0% 98%);               /* #fafafa — foreground */
    --text-muted: hsl(240 5% 64.9%);          /* #a1a1aa — muted-foreground */
    --text-dim: #94a3b8;                      /* slate-400 — labels / eyebrows */
    --border-input: hsl(240 3.7% 15.9%);      /* #27272a — border/input */
    --ring: hsl(240 4.9% 83.9%);              /* #d4d4d8 — focus ring */

    /* Glass surface (Tuesday .glass-surface, dark) */
    --glass-bg: rgba(17, 24, 39, 0.25);
    --glass-border: rgba(255, 255, 255, 0.10);
    --blur: blur(12px) saturate(1.5);
    --shadow-glass: 0 1px 2px rgba(0, 0, 0, 0.3), inset 0 1px 0 rgba(255, 255, 255, 0.05);
    --shadow-lg: 0 10px 15px -3px rgba(0, 0, 0, 0.1), 0 4px 6px -4px rgba(0, 0, 0, 0.1);

    /* Accents — the Tailwind dark shades Tuesday uses */
    --accent-primary: #3b82f6;                /* blue-500 */
    --accent-primary-deep: #2563eb;           /* blue-600 */
    --accent-primary-soft: #93c5fd;           /* blue-300 */
    --accent-success: #34d399;                /* emerald-400 */
    --accent-warning: #fbbf24;                /* amber-400 */
    --accent-danger: #f87171;                 /* red-400 */
    --tint-success: rgba(16, 185, 129, 0.10);
    --tint-warning: rgba(245, 158, 11, 0.10);
    --tint-danger: rgba(239, 68, 68, 0.10);
    --edge-success: rgba(52, 211, 153, 0.30);
    --edge-warning: rgba(251, 191, 36, 0.30);
    --edge-danger: rgba(248, 113, 113, 0.30);

    /* Nav link states (Tuesday sidebar) */
    --nav-hover: rgba(31, 41, 55, 0.5);       /* gray-800/50 */
    --nav-active: rgba(30, 58, 138, 0.3);     /* blue-900/30 */
    --nav-active-text: #bfdbfe;               /* blue-200 */

    --hairline: rgba(255, 255, 255, 0.05);    /* glass-table row divider */
    --hairline-strong: rgba(255, 255, 255, 0.2);
    --sky-hairline: rgba(56, 189, 248, 0.4);  /* signature content-top border */
    --white-5: rgba(255, 255, 255, 0.05);

    --grad-primary: linear-gradient(to right, var(--accent-primary), var(--accent-primary-deep));
    --grad-primary-hover: linear-gradient(to right, #60a5fa, #3b82f6);
    --shadow-cta: 0 4px 6px -1px rgba(30, 58, 138, 0.3), 0 2px 4px -2px rgba(30, 58, 138, 0.3);

    /* shadcn radius scale (--radius: 0.5rem) */
    --radius-2xl: 1rem;                       /* glass cards (rounded-2xl) */
    --radius-lg: 0.5rem;                      /* buttons / nav links */
    --radius-md: 0.375rem;                    /* inputs / pills / tiles */

    --focus-ring: 0 0 0 2px var(--surface), 0 0 0 4px var(--ring);
    --transition: 150ms cubic-bezier(0.4, 0, 0.2, 1);

    /* Geist Sans/Mono fallback stacks (no webfont fetch on-device) */
    --font-sans: system-ui, -apple-system, 'Segoe UI', Roboto, 'Helvetica Neue', Arial, sans-serif;
    --font-mono: ui-monospace, SFMono-Regular, Menlo, Consolas, monospace;

    --page-x: 1.5rem;
    --content-max: 80rem;                     /* max-w-7xl */
  }

  * { box-sizing: border-box; margin: 0; padding: 0; }

  /* Tuesday root sizing: 16px on phones, viewport-relative on desktop so the
     layout stays proportionally consistent across screen widths. */
  html { font-size: 16px; color-scheme: dark; forced-color-adjust: none; }
  @media (min-width: 768px) { html { font-size: clamp(13px, 0.926vw, 16px); } }

  /* Thin Tuesday scrollbars */
  * { scrollbar-width: thin; scrollbar-color: rgba(255, 255, 255, 0.1) transparent; }
  ::-webkit-scrollbar { width: 6px; height: 6px; }
  ::-webkit-scrollbar-track { background: transparent; }
  ::-webkit-scrollbar-thumb { background-color: rgba(255, 255, 255, 0.1); border-radius: 20px; }

  body {
    font-family: var(--font-sans);
    background: var(--canvas);
    color: var(--text-main);
    min-height: 100vh;
    line-height: 1.5;
    overflow-x: hidden;
    padding: 0 var(--page-x) 1rem;
    padding-bottom: max(1rem, env(safe-area-inset-bottom));
    -webkit-font-smoothing: antialiased;
    -moz-osx-font-smoothing: grayscale;
  }

  /* Sticky chrome — Tuesday's navy sidebar color as a top bar, with the
     signature sky hairline where content begins. */
  .topbar {
    position: sticky;
    top: 0;
    z-index: 50;
    margin: 0 calc(-1 * var(--page-x));
    padding: 0.75rem var(--page-x) 0;
    padding-top: max(0.75rem, env(safe-area-inset-top));   /* clear the notch/Dynamic Island (viewport-fit=cover) */
    background: var(--sidebar);
    border-bottom: 1px solid var(--sky-hairline);
  }

  header {
    display: flex;
    justify-content: space-between;
    align-items: center;
    gap: 0.75rem;
    max-width: var(--content-max);
    margin: 0 auto;
    padding: 0.25rem 0 0.75rem;
    border-bottom: 1px solid;
    border-image: linear-gradient(to right, transparent, var(--hairline-strong), transparent) 1;
  }

  .header-title { display: flex; align-items: center; gap: 0.75rem; min-width: 0; }

  /* Tuesday signature: 4px blue gradient pill beside the gradient tool heading */
  .accent-bar {
    width: 4px; height: 1.75rem; border-radius: 9999px; flex: none;
    background: linear-gradient(to bottom, var(--accent-primary), var(--accent-primary-deep));
    box-shadow: 0 1px 2px rgba(59, 130, 246, 0.4);
  }

  /* .heading-tool — white→blue-300 gradient text */
  h1 {
    font-size: 1.125rem;
    font-weight: 600;
    letter-spacing: -0.025em;
    white-space: nowrap;
    background: linear-gradient(to bottom right, #ffffff, var(--accent-primary-soft));
    -webkit-background-clip: text; background-clip: text;
    color: transparent; -webkit-text-fill-color: transparent;
  }
  @media (min-width: 640px) { h1 { font-size: 1.25rem; } }

  /* Status badge (overall summary + per-panel connection) — Tuesday tinted pill */
  .status-badge {
    display: inline-flex;
    align-items: center;
    gap: 0.4rem;
    padding: 0.25rem 0.625rem;
    background: var(--white-5);
    border: 1px solid var(--glass-border);
    border-radius: 9999px;
    font-size: 0.75rem;
    font-weight: 600;
    color: var(--text-dim);
    white-space: nowrap;
    transition: background-color var(--transition), border-color var(--transition), color var(--transition);
  }
  .status-dot { width: 6px; height: 6px; border-radius: 50%; background: var(--text-dim); flex: none; transition: background-color var(--transition); }
  .status-badge.connected { background: var(--tint-success); border-color: var(--edge-success); color: var(--accent-success); }
  .status-badge.connected .status-dot { background: var(--accent-success); }
  .status-badge.partial { background: var(--tint-warning); border-color: var(--edge-warning); color: var(--accent-warning); }
  .status-badge.partial .status-dot { background: var(--accent-warning); }
  .status-badge.disconnected { background: var(--tint-danger); border-color: var(--edge-danger); color: var(--accent-danger); }
  .status-badge.disconnected .status-dot { background: var(--accent-danger); }

  .header-actions { display: flex; align-items: center; gap: 0.5rem; flex: none; }
  /* Page footer — Backup All lives below everything */
  .page-footer {
    max-width: var(--content-max);
    margin: 1.5rem auto 0;
    display: flex;
    justify-content: center;
  }

  /* Buttons — Tuesday sidebar-action-btn (secondary) + blue gradient CTA (primary) */
  button.btn {
    display: inline-flex; align-items: center; justify-content: center; white-space: nowrap;
    padding: 0.5rem 0.75rem;
    background: rgba(17, 24, 39, 0.5);
    border: 1px solid rgba(255, 255, 255, 0.15);
    border-radius: var(--radius-lg);
    color: var(--text-main);
    font-family: inherit; font-size: 0.875rem; font-weight: 500;
    cursor: pointer;
    backdrop-filter: var(--blur); -webkit-backdrop-filter: var(--blur);
    box-shadow: var(--shadow-lg);
    transition: background-color var(--transition), border-color var(--transition), color var(--transition), box-shadow var(--transition);
  }
  button.btn:hover { background: rgba(17, 24, 39, 0.8); border-color: rgba(255, 255, 255, 0.3); }
  button.btn:focus-visible { outline: none; box-shadow: var(--focus-ring); }
  button.btn:disabled { pointer-events: none; opacity: 0.5; }
  button.btn-primary {
    background: var(--grad-primary);
    border-color: transparent;
    color: #fff;
    box-shadow: var(--shadow-cta);
  }
  button.btn-primary:hover { background: var(--grad-primary-hover); border-color: transparent; }
  button.btn-primary:disabled { background: #374151; color: #9ca3af; box-shadow: none; opacity: 1; }

  /* Machine tab strip — Tuesday sidebar nav links laid horizontally on the navy
     chrome: idle muted, gray-800/50 hover, blue-900/30 active with a 4px
     blue-500 indicator bar, plus live connectivity dots. */
  .tabs {
    display: flex;
    gap: 0.25rem;
    max-width: var(--content-max);
    margin: 0 auto;
    padding: 0.5rem 0;
    overflow-x: auto;
    scrollbar-width: none;
  }
  .tabs::-webkit-scrollbar { display: none; }
  .tab {
    position: relative;
    overflow: hidden;                  /* clip the active indicator bar to the rounded corners */
    display: flex; align-items: center; gap: 0.5rem;
    flex: 1 0 auto; justify-content: center;
    padding: 0.625rem 0.75rem;
    border: none;
    border-radius: var(--radius-lg);
    background: transparent;
    color: var(--text-muted);
    font-family: inherit;
    font-size: 0.875rem; font-weight: 500;
    white-space: nowrap; cursor: pointer;
    transition: background-color 200ms cubic-bezier(0.4, 0, 0.2, 1), color 200ms cubic-bezier(0.4, 0, 0.2, 1);
  }
  .tab:hover { background: var(--nav-hover); color: var(--text-main); }
  .tab:focus-visible { outline: none; box-shadow: var(--focus-ring); }
  .tab.active { background: var(--nav-active); color: var(--nav-active-text); }
  .tab.active::before {
    content: '';
    position: absolute; top: 0; bottom: 0; left: 0; width: 4px;
    background: var(--accent-primary);
    border-radius: 0 9999px 9999px 0;
  }
  .tdot { width: 6px; height: 6px; border-radius: 50%; background: #64748b; flex: none; transition: background-color var(--transition); }
  .tdot.online { background: var(--accent-success); }

  /* Layout — swipeable carousel: one full-width machine "card" per page.
     Horizontal scroll-snap drives the swipe; the tab strip stays in sync as
     both position indicator and quick-jump. Height tracks the active page so
     short machines don't leave a gap (expands to the tallest mid-swipe to
     avoid clipping the incoming page). */
  .stage {
    max-width: var(--content-max);
    margin: 1rem auto 0;
    position: relative;
    z-index: 1;
    display: flex;
    align-items: flex-start;
    gap: 0;
    overflow-x: auto;
    overflow-y: hidden;
    scroll-snap-type: x mandatory;
    -webkit-overflow-scrolling: touch;
    overscroll-behavior-x: contain;
    scrollbar-width: none;
    transition: height 0.28s cubic-bezier(0.22, 0.61, 0.36, 1);
  }
  .stage::-webkit-scrollbar { display: none; }
  .dashboard-grid { display: grid; grid-template-columns: repeat(12, 1fr); gap: 1rem; }
  .machine-panel {
    flex: 0 0 100%;
    width: 100%;
    min-width: 0;
    padding-bottom: 0.5rem;            /* slack so the bottom card's shadow isn't clipped */
    scroll-snap-align: start;
    scroll-snap-stop: always;          /* one machine per swipe, never skip */
  }

  .col-span-4 { grid-column: span 4; }
  .col-span-6 { grid-column: span 6; }
  .col-span-8 { grid-column: span 8; }
  .col-span-12 { grid-column: span 12; }

  /* Cards — Tuesday .glass-surface sections (rounded-2xl glass-surface p-5) */
  .card {
    background: var(--glass-bg);
    border: 1px solid var(--glass-border);
    border-radius: var(--radius-2xl);
    padding: 1.25rem;
    position: relative;
    backdrop-filter: var(--blur);
    -webkit-backdrop-filter: var(--blur);
    box-shadow: var(--shadow-glass);
    display: flex; flex-direction: column; overflow: visible;
    transition: opacity var(--transition);
  }
  .machine-panel.offline .card { opacity: 0.55; }

  /* Card title — Tuesday stats-card header: sm semibold uppercase slate-400 */
  .card-header {
    display: flex; align-items: center; gap: 0.5rem;
    margin-bottom: 1rem;
    color: var(--text-dim);
    font-size: 0.875rem; font-weight: 600;
    text-transform: uppercase; letter-spacing: 0.05em;
  }
  .card-header svg { width: 16px; height: 16px; stroke-width: 1.8; color: var(--text-dim); }

  /* Typography — Tuesday stat-tile scale */
  .value-huge { font-size: 1.875rem; font-weight: 700; line-height: 1; color: var(--text-main); font-variant-numeric: tabular-nums; }
  .label-sm { font-size: 0.625rem; color: var(--text-dim); font-weight: 600; margin-top: 0.375rem; text-transform: uppercase; letter-spacing: 0.05em; }
  .mono { font-family: var(--font-mono); }

  /* Hero (System Overview) */
  .state-row { display: flex; align-items: center; gap: 0.5rem; flex-wrap: wrap; margin-bottom: 0.75rem; }
  .state-pill {
    display: inline-flex; align-items: center;
    padding: 0.125rem 0.5rem;
    background: var(--nav-active);
    color: var(--nav-active-text);
    border-radius: var(--radius-md);
    font-size: 0.6875rem; font-weight: 600; letter-spacing: 0.025em; text-transform: uppercase;
  }
  .badge { padding: 0.125rem 0.625rem; border-radius: 9999px; font-size: 0.75rem; font-weight: 600; letter-spacing: 0.025em; text-transform: uppercase; border: 1px solid transparent; }
  .badge.HEALTHY { background: var(--tint-success); color: var(--accent-success); border-color: var(--edge-success); }
  .badge.WARNING { background: var(--tint-warning); color: var(--accent-warning); border-color: var(--edge-warning); }
  .badge.ERROR { background: var(--tint-danger); color: var(--accent-danger); border-color: var(--edge-danger); }
  .badge.UNK { background: var(--white-5); color: var(--text-dim); border-color: var(--glass-border); }

  .hero-stats { display: grid; grid-template-columns: repeat(auto-fit, minmax(0, 1fr)); gap: 0; padding: 0.75rem 0 0.25rem; }
  .hero-stat { text-align: center; padding: 0.25rem 0.75rem; }
  .hero-stat + .hero-stat { border-left: 1px solid var(--hairline); }
  .cut-rates { border-top: 1px solid var(--hairline); margin-top: 0.5rem; padding-top: 0.625rem; }
  .cut-rates-cap { font-size: 0.625rem; color: var(--text-dim); font-weight: 600; text-transform: uppercase; letter-spacing: 0.05em; text-align: center; margin-bottom: 0.5rem; }
  .cut-rate-row { display: grid; grid-template-columns: repeat(4, 1fr); gap: 0; }
  .cut-rate-cell { text-align: center; padding: 0.25rem 0.5rem; }
  .cut-rate-cell + .cut-rate-cell { border-left: 1px solid var(--hairline); }
  .cut-rate-value { font-size: 1.25rem; font-weight: 700; line-height: 1; color: var(--text-main); font-variant-numeric: tabular-nums; }

  /* Connection / Diagnostics — Tuesday glass-table rows: hairline dividers, white/5 hover */
  .stat-row {
    display: flex; align-items: center; justify-content: space-between;
    padding: 0.5rem 0.75rem;
    border-top: 1px solid var(--hairline);
    transition: background-color var(--transition);
  }
  .stat-row:first-child { border-top: 0; }
  .stat-row:hover { background: var(--white-5); }
  .stat-row .k { font-size: 0.875rem; color: var(--text-dim); font-weight: 500; }
  .stat-row .v { font-size: 0.875rem; color: var(--text-main); font-family: var(--font-mono); font-variant-numeric: tabular-nums; }

  /* Sensor Matrix — white/5 tiles, emerald tint when active (no glow) */
  .sensor-grid { display: grid; grid-template-columns: repeat(2, 1fr); gap: 0.5rem; }
  .sensor-item {
    background: var(--white-5);
    padding: 0.5rem 0.75rem;
    border-radius: var(--radius-md);
    display: flex; align-items: center; justify-content: space-between;
    border: 1px solid transparent;
    transition: background-color var(--transition), border-color var(--transition);
  }
  .sensor-item:hover { background: rgba(255, 255, 255, 0.08); }
  .sensor-item.active { background: var(--tint-success); border-color: var(--edge-success); }
  .sensor-label { font-size: 0.875rem; font-weight: 500; color: var(--text-muted); }
  .sensor-item.active .sensor-label { color: var(--text-main); }
  .sensor-val { font-size: 0.75rem; font-family: var(--font-mono); color: var(--text-main); font-variant-numeric: tabular-nums; }
  .sensor-led { width: 6px; height: 6px; border-radius: 50%; background: rgba(255, 255, 255, 0.15); transition: background-color var(--transition); flex: none; }
  .sensor-item.active .sensor-led { background: var(--accent-success); }

  /* Mode Toggle — exact Tuesday PageToggle (Orders/Planner switcher): gray-800/60
     track with inset gray-700/60 ring, gap-1 between options, sliding neutral
     gray-700 pill with shadow-sm (NOT a blue pill). 216ms slide + squish. */
  .mode-toggle-container { display: flex; justify-content: center; margin-bottom: 1rem; }
  .mode-toggle {
    position: relative; display: inline-grid; grid-template-columns: repeat(2, 1fr);
    gap: 4px; padding: 4px; border-radius: 9999px;          /* gap-1, p-1, rounded-full */
    background: rgba(31, 41, 55, 0.6);                      /* dark:bg-gray-800/60 */
    box-shadow: inset 0 0 0 1px rgba(55, 65, 81, 0.6);      /* ring-1 ring-inset ring-gray-700/60 */
  }
  .mode-bubble-track {
    position: absolute; top: 4px; bottom: 4px; left: 4px;   /* top-1 bottom-1 */
    width: calc((100% - 12px) / 2);                         /* (100% - PILL_GAP*3) / 2 */
    transform: translateX(0);
    transition: transform 216ms cubic-bezier(0.32, 1.2, 0.55, 1);
    z-index: 1; pointer-events: none; will-change: transform;
  }
  .mode-toggle.mode-1 .mode-bubble-track { transform: translateX(calc(100% + 4px)); }
  .mode-bubble {
    width: 100%; height: 100%; border-radius: 9999px;
    background: #374151;                                    /* dark:bg-gray-700 */
    box-shadow: 0 1px 2px 0 rgba(0, 0, 0, 0.05);           /* shadow-sm */
    transform-origin: center;
  }
  .mode-bubble.squish { animation: pill-squish 420ms cubic-bezier(0.34, 1.4, 0.5, 1); }
  @keyframes pill-squish {
    0%   { transform: scaleX(1)    scaleY(1); }
    30%  { transform: scaleX(0.78) scaleY(1.12); }
    60%  { transform: scaleX(1.06) scaleY(0.95); }
    82%  { transform: scaleX(0.98) scaleY(1.02); }
    100% { transform: scaleX(1)    scaleY(1); }
  }
  .mode-option {
    position: relative; z-index: 2; height: 2rem; padding: 0 1rem; border-radius: 9999px;
    cursor: pointer; display: inline-flex; align-items: center; justify-content: center;
    font-size: 0.875rem; font-weight: 500; color: #9ca3af;
    transition: color 200ms cubic-bezier(0.4, 0, 0.2, 1); user-select: none;
  }
  .mode-option.active { color: #fff; }
  .mode-option:hover:not(.active) { color: #e5e7eb; }

  /* Collapsible config section — Tuesday stat-tile label as a clickable header
     with a rotating chevron; body collapses via a grid-rows 1fr→0fr transition. */
  .cfg-section { display: flex; flex-direction: column; }
  .section-heading {
    display: flex; align-items: center; gap: 0.35rem;
    font-size: 0.625rem; color: var(--text-dim); font-weight: 600;
    letter-spacing: 0.05em; text-transform: uppercase;
    margin: 0.75rem 0 0.5rem; padding-top: 0.625rem;
    border-top: 1px solid var(--hairline);
    cursor: pointer; user-select: none;
    transition: color var(--transition);
  }
  .section-heading:hover { color: var(--text-muted); }
  .cfg-section:first-child .section-heading { border-top: 0; padding-top: 0; margin-top: 0; }
  .sec-chevron { width: 13px; height: 13px; flex: none; stroke-width: 2.5; color: var(--text-dim); transition: transform 0.22s cubic-bezier(0.4, 0, 0.2, 1); }
  .cfg-section.collapsed .sec-chevron { transform: rotate(-90deg); }
  .cfg-section-body { display: grid; grid-template-rows: 1fr; transition: grid-template-rows 0.22s cubic-bezier(0.4, 0, 0.2, 1); }
  .cfg-section.collapsed .cfg-section-body { grid-template-rows: 0fr; }
  .cfg-section-inner { overflow: hidden; min-height: 0; }
  .cfg-section.collapsed .section-heading { margin-bottom: 0.25rem; }

  /* Config inputs — shadcn Input recipe */
  .input-group { margin-bottom: 0.625rem; }
  .input-group label { display: block; font-size: 0.875rem; color: var(--text-main); margin-bottom: 0.375rem; font-weight: 500; line-height: 1.25; }
  .input-row { display: flex; gap: 0.5rem; align-items: center; }
  .input-group input[type="number"]::-webkit-outer-spin-button,
  .input-group input[type="number"]::-webkit-inner-spin-button { -webkit-appearance: none; margin: 0; }
  .input-group input[type="number"] { -moz-appearance: textfield; appearance: textfield; }
  input[type="number"] {
    height: 2.5rem;
    background: var(--surface);
    border: 1px solid var(--border-input);
    color: var(--text-main);
    padding: 0.5rem 0.75rem;
    border-radius: var(--radius-md);
    width: 100%;
    font-family: inherit;
    font-size: 0.875rem; font-variant-numeric: tabular-nums;
    transition: border-color var(--transition), box-shadow var(--transition);
  }
  input:focus { outline: none; box-shadow: var(--focus-ring); }
  input:disabled { opacity: 0.5; cursor: not-allowed; }
  input.err { border-color: var(--accent-danger); }

  /* Per-field save status icon */
  .fstat { width: 1.25rem; flex: none; display: flex; align-items: center; justify-content: center; font-size: 0.875rem; line-height: 1; }
  .fstat.ok { color: var(--accent-success); } .fstat.warn { color: var(--accent-warning); } .fstat.err { color: var(--accent-danger); }
  .spin { width: 0.875rem; height: 0.875rem; border: 2px solid var(--border-input); border-top-color: var(--accent-primary); border-radius: 50%; animation: spin 0.7s linear infinite; }
  @keyframes spin { to { transform: rotate(360deg); } }
  .fade { animation: fade 2.4s forwards; }
  @keyframes fade { 0%,55% { opacity: 1; } 100% { opacity: 0; } }

  .cfg-status { font-size: 0.75rem; text-align: center; min-height: 1.2em; transition: color 0.25s; margin: 0.25rem 0 0; }
  .cfg-empty { font-size: 0.875rem; color: var(--text-dim); padding: 0.5rem 0; }

  /* Pin the tall Config card to the right column, spanning 2 rows. */
  .config-card { grid-column: 9 / -1; grid-row: 1 / span 2; }

  .fade-in { animation: fadeIn 0.4s ease forwards; }
  @keyframes fadeIn { from { opacity: 0; transform: translateY(6px); } to { opacity: 1; transform: translateY(0); } }

  /* Responsive */
  @media (max-width: 1024px) {
    .dashboard-grid { grid-template-columns: 1fr 1fr; }
    .col-span-4, .col-span-6, .col-span-8, .col-span-12 { grid-column: span 2; }
    .config-card { grid-row: auto; }
  }
  @media (max-width: 768px) {
    :root { --page-x: 0.75rem; }
    /* Machine selector: 2x2 grid so all four tabs fit a vertical phone, no side-scroll. */
    .tabs { display: grid; grid-template-columns: 1fr 1fr; overflow-x: visible; }
    .tab { flex: none; min-width: 0; padding: 0.625rem 0.5rem; }
    .tab span { overflow: hidden; text-overflow: ellipsis; white-space: nowrap; }
    .dashboard-grid { grid-template-columns: 1fr; gap: 0.75rem; }
    .col-span-4, .col-span-6, .col-span-8, .col-span-12 { grid-column: span 1; }
    .sensor-grid { grid-template-columns: 1fr; }
    .hero-stats { grid-template-columns: 1fr; }
    .hero-stat + .hero-stat { border-left: 0; border-top: 1px solid var(--hairline); margin-top: 0.5rem; padding-top: 0.625rem; }
    /* Mobile stacking: System Overview → Configuration → everything else. */
    .machine-panel .card { order: 3; }
    .machine-panel .hero-card { order: 1; }
    .machine-panel .config-card { order: 2; }
    /* Phones: drop live backdrop-blur on the swiping cards — re-sampling it every
       scroll frame is what makes the carousel feel laggy. The canvas is flat
       gray-900, so a near-opaque tint looks identical. */
    .card { backdrop-filter: none; -webkit-backdrop-filter: none; background: rgba(17, 24, 39, 0.9); }
    button.btn { backdrop-filter: none; -webkit-backdrop-filter: none; }
  }
</style>
</head>
<body>
<div class="topbar">
  <header>
    <div class="header-title">
      <div class="accent-bar"></div>
      <h1>Production Line</h1>
    </div>
    <div class="header-actions">
      <div class="status-badge disconnected" id="summaryBadge">
        <div class="status-dot"></div>
        <span id="sum-txt">connecting…</span>
      </div>
    </div>
  </header>
  <div class="tabs" id="tabs"></div>
</div>
<div class="stage" id="stage"></div>
<footer class="page-footer">
  <button class="btn btn-primary" id="backupBtn">↓ Backup All</button>
</footer>
<script>
const MACHINES = [
  { id:"stage1", name:"Stage 1",      base:"http://192.168.1.251", hasMode:true },
  { id:"ta",     name:"Transfer Arm", base:"" },
  { id:"stage2", name:"Stage 2",      base:"http://192.168.1.253" },
  { id:"router", name:"Router",       base:"http://192.168.1.250" },
];
const DEFAULT_TAB = "stage1";
const POLL_MS = 2000, TIMEOUT_MS = 1500;
const STAGE_GLIDE_MS = 360;  // tab-tap page glide duration (scroll + height animate together)
const RECONCILE_EVERY = 8;  // every Nth poll, re-pull /api/config to catch reboots + applied deferrals
const UPTIME_RESYNC_MS = 5000;  // device-clock gap beyond this = reboot/long gap → hard re-anchor the uptime readout
const UPTIME_SLEW = 0.05;       // otherwise nudge the anchor this fraction toward the device clock (absorbs latency jitter + drift)
const ST = {};              // id -> per-machine runtime state (DOM refs + flags)
let active = DEFAULT_TAB;
let pollCount = 0;
let maxPanelH = 0;          // tallest panel — used to avoid clipping the incoming page mid-swipe
let stageMoving = false;    // true while the carousel is being swiped/animated
let animFrame = null;       // rAF id while a tab-tap glide is animating (null = manual swipe / idle)
let relayoutQueued = false;
let settleTimer = null;

function el(tag, cls, txt) { const e = document.createElement(tag); if (cls) e.className = cls; if (txt != null) e.textContent = txt; return e; }
function svg(paths) {
  const ns = "http://www.w3.org/2000/svg";
  const s = document.createElementNS(ns, "svg");
  s.setAttribute("fill", "none"); s.setAttribute("stroke", "currentColor"); s.setAttribute("viewBox", "0 0 24 24");
  paths.forEach(d => { const p = document.createElementNS(ns, "path"); p.setAttribute("stroke-linecap", "round"); p.setAttribute("stroke-linejoin", "round"); p.setAttribute("d", d); s.appendChild(p); });
  return s;
}
// Down-chevron for a collapsible section header (rotates to point right when collapsed).
function chevron() { const c = svg(["M6 9l6 6 6-6"]); c.classList.add("sec-chevron"); return c; }
// Round to 2 decimals — used when converting step-based speeds to/from inches/sec for display.
function round2(x) { return Math.round(x * 100) / 100; }
// Turn an API sensor key (camelCase / snake_case) into a readable label.
function humanize(k) {
  return String(k)
    .replace(/^_+/, "")
    .replace(/_/g, " ")
    .replace(/([a-z\d])([A-Z])/g, "$1 $2")
    .replace(/([A-Z]+)([A-Z][a-z])/g, "$1 $2")
    .replace(/\s+/g, " ").trim()
    .replace(/\b\w/g, c => c.toUpperCase());
}
function fmtUptime(ms) {
  const s = Math.floor(ms / 1000), d = Math.floor(s / 86400), h = Math.floor((s % 86400) / 3600), m = Math.floor((s % 3600) / 60), sec = s % 60;
  if (d > 0) return d + "d " + h + "h " + m + "m";
  return String(h).padStart(2, "0") + ":" + String(m).padStart(2, "0") + ":" + String(sec).padStart(2, "0");
}
// Anchor the device's boot instant in local-clock terms, then count up locally so the
// readout ticks smoothly. Re-anchor only on a real discontinuity (reboot / long gap);
// absorb per-poll latency jitter + slow clock drift with a gentle slew so it never
// stutters backward between polls.
function syncUptime(s, uptimeMs) {
  const implied = performance.now() - uptimeMs;   // local time at which the device booted
  if (s.uptimeEpoch == null || Math.abs(implied - s.uptimeEpoch) > UPTIME_RESYNC_MS) s.uptimeEpoch = implied;
  else s.uptimeEpoch += (implied - s.uptimeEpoch) * UPTIME_SLEW;
}

async function fetchJSON(url, opts) {
  const ctrl = new AbortController();
  const t = setTimeout(() => ctrl.abort(), TIMEOUT_MS);
  try {
    const r = await fetch(url, Object.assign({ signal:ctrl.signal }, opts || {}));
    const body = await r.text();
    let json = null; try { json = JSON.parse(body); } catch (e) {}
    return { ok:r.ok, status:r.status, json };
  } finally { clearTimeout(t); }
}

// Panel construction — one navy card-grid per machine.
function buildPanel(m) {
  const s = ST[m.id] = { m, online:false, configLoaded:false, configLoading:false, mode:null, uptimeEpoch:null, uptimeText:null };
  const panel = el("div", "machine-panel");
  const grid = el("div", "dashboard-grid");

  // --- Hero / System Overview (col-span-8) ---
  const hero = el("div", "card hero-card col-span-8 fade-in");
  const hHead = el("div", "card-header");
  hHead.style.justifyContent = "space-between";
  const hLeft = el("div"); hLeft.style.display = "flex"; hLeft.style.alignItems = "center"; hLeft.style.gap = "0.5rem";
  hLeft.appendChild(svg(["M13 10V3L4 14h7v7l9-11h-7z"]));
  hLeft.appendChild(el("span", null, m.name));
  s.connBadge = el("div", "status-badge disconnected");
  s.connBadge.appendChild(el("div", "status-dot"));
  s.connText = el("span", null, "Offline"); s.connBadge.appendChild(s.connText);
  hHead.appendChild(hLeft); hHead.appendChild(s.connBadge);
  hero.appendChild(hHead);

  const stateRow = el("div", "state-row");
  s.statePill = el("div", "state-pill", "—");
  s.healthBadge = el("div", "badge UNK", "—");
  stateRow.appendChild(s.statePill); stateRow.appendChild(s.healthBadge);
  hero.appendChild(stateRow);

  const stats = el("div", "hero-stats");
  const up = el("div", "hero-stat");
  s.uptime = el("div", "value-huge mono", "--:--:--"); up.appendChild(s.uptime); up.appendChild(el("div", "label-sm", "UPTIME"));
  stats.appendChild(up);
  hero.appendChild(stats);

  // Cuts-per-minute (rolling 1/3/5/15-min averages). Only shown for machines
  // that report a cutRates object in their status (e.g. Stage 1).
  s.cutRates = el("div", "cut-rates"); s.cutRates.style.display = "none";
  s.cutRates.appendChild(el("div", "cut-rates-cap", "Cuts / Min — Rolling Avg"));
  const crRow = el("div", "cut-rate-row");
  s.cutCells = {};
  [["1 MIN","m1"],["3 MIN","m3"],["5 MIN","m5"],["15 MIN","m15"]].forEach(([lbl, key]) => {
    const cell = el("div", "cut-rate-cell");
    s.cutCells[key] = el("div", "cut-rate-value mono", "—");
    cell.appendChild(s.cutCells[key]); cell.appendChild(el("div", "label-sm", lbl));
    crRow.appendChild(cell);
  });
  s.cutRates.appendChild(crRow);
  hero.appendChild(s.cutRates);
  grid.appendChild(hero);

  // --- Configuration (right column, tall) ---
  const cfg = el("div", "card col-span-4 config-card fade-in");
  const cHead = el("div", "card-header");
  cHead.appendChild(svg([
    "M10.325 4.317c.426-1.756 2.924-1.756 3.35 0a1.724 1.724 0 002.573 1.066c1.543-.94 3.31.826 2.37 2.37a1.724 1.724 0 001.065 2.572c1.756.426 1.756 2.924 0 3.35a1.724 1.724 0 00-1.066 2.573c.94 1.543-.826 3.31-2.37 2.37a1.724 1.724 0 00-2.572 1.065c-.426 1.756-2.924 1.756-3.35 0a1.724 1.724 0 00-2.573-1.066c-1.543.94-3.31-.826-2.37-2.37a1.724 1.724 0 00-1.065-2.572c-1.756-.426-1.756-2.924 0-3.35a1.724 1.724 0 001.066-2.573c-.94-1.543.826-3.31 2.37-2.37.996.608 2.296.07 2.572-1.065z",
    "M15 12a3 3 0 11-6 0 3 3 0 016 0z"
  ]));
  cHead.appendChild(el("span", null, "Configuration"));
  cfg.appendChild(cHead);

  // Stage 1 dual-mode toggle (3 Inch / Minis) — drives the reserved __mode__ key.
  if (m.hasMode) {
    const wrap = el("div", "mode-toggle-container");
    const toggle = el("div", "mode-toggle mode-0");
    const track = el("div", "mode-bubble-track");
    s.modeBubble = el("div", "mode-bubble"); track.appendChild(s.modeBubble);
    const opt0 = el("div", "mode-option active", "3 Inch");
    const opt1 = el("div", "mode-option", "Minis");
    opt0.onclick = () => setMachineMode(s, 0);
    opt1.onclick = () => setMachineMode(s, 1);
    toggle.appendChild(track); toggle.appendChild(opt0); toggle.appendChild(opt1);
    wrap.appendChild(toggle);
    cfg.appendChild(wrap);
    s.modeToggle = toggle; s.modeOpt0 = opt0; s.modeOpt1 = opt1;
  }

  s.fields = el("div");
  s.fields.appendChild(el("div", "cfg-empty", "waiting for connection…"));
  cfg.appendChild(s.fields);
  s.cfgStatus = el("div", "cfg-status"); cfg.appendChild(s.cfgStatus);
  grid.appendChild(cfg);

  // --- Sensor Matrix (col-span-4) ---
  const sens = el("div", "card col-span-4 fade-in");
  const sHead = el("div", "card-header");
  sHead.appendChild(svg(["M15 12a3 3 0 11-6 0 3 3 0 016 0z", "M2.458 12C3.732 7.943 7.523 5 12 5c4.478 0 8.268 2.943 9.542 7-1.274 4.057-5.064 7-9.542 7-4.477 0-8.268-2.943-9.542-7z"]));
  sHead.appendChild(el("span", null, "Sensor Matrix"));
  sens.appendChild(sHead);
  s.sensors = el("div", "sensor-grid");
  s.sensors.appendChild(el("div", "cfg-empty", "—"));
  sens.appendChild(s.sensors);
  grid.appendChild(sens);

  // --- Connection / Diagnostics (col-span-4) ---
  const diag = el("div", "card col-span-4 fade-in");
  const dHead = el("div", "card-header");
  dHead.appendChild(svg(["M8.111 16.404a5.5 5.5 0 017.778 0M5.282 13.575a9.5 9.5 0 0113.436 0M1.394 9.686a15 15 0 0121.212 0M12 20h.01"]));
  dHead.appendChild(el("span", null, "Connection"));
  diag.appendChild(dHead);
  const wrap = el("div");
  s.dHealth = mkStatRow(wrap, "Health", "—");
  mkStatRow(wrap, "Address", m.base || "same origin");
  diag.appendChild(wrap);
  grid.appendChild(diag);

  panel.appendChild(grid);
  s.panel = panel;
  return panel;
}

function mkStatRow(parent, key, val) {
  const row = el("div", "stat-row");
  row.appendChild(el("span", "k", key));
  const v = el("span", "v", val);
  row.appendChild(v); parent.appendChild(row);
  return v;
}

// Carousel navigation. The machine pages are a horizontal scroll-snap track;
// "active" is whichever page is snapped into view. Tabs mirror it and can jump.
function indexOf(id) { for (let i = 0; i < MACHINES.length; i++) if (MACHINES[i].id === id) return i; return 0; }

function markActive(id) {
  active = id;
  MACHINES.forEach(m => ST[m.id].tab.classList.toggle("active", m.id === id));
}

// scroll: false = just sync UI, "smooth" = animate to the page, "instant" = jump.
function setActive(id, scroll) {
  markActive(id);
  const targetLeft = indexOf(id) * stage.clientWidth;
  if (scroll && Math.abs(stage.scrollLeft - targetLeft) > 1) {
    if (scroll === "smooth") {
      // Tab-tap jump. Snap off so the glide runs straight to the target instead of
      // stuttering through every intermediate snap stop, and resize the track toward
      // the destination *in step* with the slide so the page doesn't slide sideways
      // then visibly collapse down as a jarring second move.
      stageMoving = true;
      stage.style.scrollSnapType = "none";
      const targetH = ST[id] && ST[id]._h;
      stage.style.transition = "height " + STAGE_GLIDE_MS + "ms cubic-bezier(0.22, 0.61, 0.36, 1)";
      if (targetH) stage.style.height = targetH + "px";
      animateStage(targetLeft);             // drive scrollLeft ourselves — see animateStage
      return;
    }
    stage.scrollTo({ left: targetLeft, behavior: "auto" });   // "instant" jump
  }
  applyHeight();
}

// Glide the carousel to a scroll offset by animating scrollLeft ourselves. We do
// NOT use scrollTo({behavior:"smooth"}) because native smooth scroll on a snap
// sub-scroller is unreliable on mobile Safari (it jumps or stutters); a manual
// rAF tween is smooth everywhere and can't be fought by scroll-snap.
function animateStage(targetLeft) {
  if (animFrame) cancelAnimationFrame(animFrame);
  const start = stage.scrollLeft, dist = targetLeft - start;
  let t0 = null;
  function frame(now) {
    if (t0 === null) t0 = now;
    let p = (now - t0) / STAGE_GLIDE_MS;
    if (p > 1) p = 1;
    const e = p < 0.5 ? 4 * p * p * p : 1 - Math.pow(-2 * p + 2, 3) / 2;  // easeInOutCubic
    stage.scrollLeft = start + dist * e;
    if (p < 1) { animFrame = requestAnimationFrame(frame); }
    else { animFrame = null; endStageMove(); }
  }
  animFrame = requestAnimationFrame(frame);
}

// Restore the resting state after a swipe/jump: re-engage snap, drop the inline
// transition (back to the CSS default) and size the track to the active page.
function endStageMove() {
  stageMoving = false;
  stage.style.scrollSnapType = "";
  stage.style.transition = "";
  applyHeight();
}

// Size the carousel to the active page so a short machine leaves no gap below.
// Skipped while a swipe is in flight (the track is held at full height then).
function applyHeight() {
  if (stageMoving) return;
  const h = ST[active] && ST[active]._h;
  if (h) stage.style.height = h + "px";
}

// Re-measure each page's natural height (align-items:flex-start keeps panels
// content-sized regardless of the track's fixed height). rAF-debounced; call
// after anything that changes a panel's content.
function relayout() {
  relayoutQueued = false;
  let mx = 0;
  MACHINES.forEach(m => { const h = ST[m.id].panel.offsetHeight; ST[m.id]._h = h; if (h > mx) mx = h; });
  maxPanelH = mx;
  applyHeight();
}
function scheduleRelayout() { if (!relayoutQueued) { relayoutQueued = true; requestAnimationFrame(relayout); } }

// Keep the active tab + height in sync as the user swipes between pages.
function onStageScroll() {
  if (!stageMoving) {                                  // entering motion: hold full height, no clipping
    stageMoving = true;
    stage.style.transition = "none";
    if (maxPanelH) stage.style.height = maxPanelH + "px";
  }
  if (!animFrame) {                                    // manual swipe: scroll position drives the active tab
    const i = Math.max(0, Math.min(MACHINES.length - 1, Math.round(stage.scrollLeft / stage.clientWidth)));
    if (MACHINES[i].id !== active) markActive(MACHINES[i].id);
  }                                                    // (a tab-tap glide already marked its target up front)
  clearTimeout(settleTimer);
  settleTimer = setTimeout(endStageMove, 130);         // motion settled: re-snap + shrink to the active page
}

// Live status poll
async function poll(m) {
  const s = ST[m.id];
  let r = null;
  try { r = await fetchJSON(m.base + "/api/status"); } catch (e) {}
  if (!r || !r.ok || !r.json) { setOffline(s); return; }
  setOnline(s, r.json);
  if (!s.configLoaded && !s.configLoading) loadConfig(s);  // auto-load curated fields once online
}

function setOnline(s, d) {
  s.online = true;
  s.panel.classList.remove("offline");
  s.tabDot.className = "tdot online";
  s.connBadge.className = "status-badge connected"; s.connText.textContent = "Online";
  s.statePill.textContent = d.state || "?";
  const h = d.health || "UNK";
  s.healthBadge.className = "badge " + (["HEALTHY","WARNING","ERROR"].includes(h) ? h : "UNK");
  s.healthBadge.textContent = h;

  if (d.uptimeMs != null) syncUptime(s, d.uptimeMs);   // rAF loop paints it; only re-anchors on reboot

  if (d.cutRates) {
    s.cutRates.style.display = "";
    const fmt = x => x == null ? "—" : (Math.round(x * 10) / 10).toFixed(1);
    ["m1","m3","m5","m15"].forEach(k => s.cutCells[k].textContent = fmt(d.cutRates[k]));
  }

  s.dHealth.textContent = h;
  s.dHealth.style.color = h === "HEALTHY" ? "var(--accent-success)" : h === "WARNING" ? "var(--accent-warning)" : h === "ERROR" ? "var(--accent-danger)" : "var(--text-main)";

  renderSensors(s, d.sensors || {});

  if (s.m.hasMode && d.mode != null && s._modePending == null) updateModeUI(s, d.mode, false);

  s.fields.querySelectorAll("input").forEach(i => i.disabled = false);  // re-enable once reachable
}

function setOffline(s) {
  s.online = false;
  s.panel.classList.add("offline");
  s.tabDot.className = "tdot";
  s.connBadge.className = "status-badge disconnected"; s.connText.textContent = "Offline";
  s.statePill.textContent = "Offline";
  s.healthBadge.className = "badge UNK"; s.healthBadge.textContent = "—";
  s.uptimeText = null; s.uptime.textContent = "--:--:--";   // keep uptimeEpoch so a transient drop resumes smoothly
  s.dHealth.textContent = "—"; s.dHealth.style.color = "var(--text-dim)";
  s.fields.querySelectorAll("input").forEach(i => i.disabled = true);
  scheduleRelayout();
}

function renderSensors(s, map) {
  const keys = Object.keys(map);
  s.sensors.innerHTML = "";
  if (!keys.length) { s.sensors.appendChild(el("div", "cfg-empty", "no sensors")); scheduleRelayout(); return; }
  keys.forEach(k => {
    const v = map[k];
    const item = el("div", "sensor-item");
    item.appendChild(el("span", "sensor-label", humanize(k)));
    if (typeof v === "boolean") {
      if (v) item.classList.add("active");
      item.appendChild(el("div", "sensor-led"));
    } else {
      item.appendChild(el("span", "sensor-val", String(v)));
    }
    s.sensors.appendChild(item);
  });
  scheduleRelayout();   // sensor count may have changed the page height
}

// Curated settings
async function loadConfig(s) {
  s.configLoading = true;
  let r = null;
  try { r = await fetchJSON(s.m.base + "/api/config"); } catch (e) {}
  s.configLoading = false;
  if (!r || !r.ok || !r.json || !Array.isArray(r.json.fields)) return;  // retry on a later poll
  s.configLoaded = true;
  // steps/inch lets fromSteps fields render + edit in inches/sec (front-end only;
  // POST still sends steps). 0 when the machine doesn't advertise it → no conversion.
  const spi = Number(r.json.stepsPerInch) || 0;
  s.fields.innerHTML = "";
  if (!r.json.fields.length) { s.fields.appendChild(el("div", "cfg-empty", "no settings")); scheduleRelayout(); return; }
  let lastGroup = null, inner = null;
  r.json.fields.forEach(f => {
    if (f.group && f.group !== lastGroup) {  // start a new collapsible section
      const sec = el("div", "cfg-section" + (f.collapsed ? " collapsed" : ""));
      const head = el("div", "section-heading");
      head.appendChild(chevron());
      head.appendChild(el("span", null, f.group));
      head.onclick = () => { sec.classList.toggle("collapsed"); scheduleRelayout(); setTimeout(scheduleRelayout, 240); };
      const body = el("div", "cfg-section-body");
      inner = el("div", "cfg-section-inner");
      body.appendChild(inner);
      sec.appendChild(head); sec.appendChild(body);
      s.fields.appendChild(sec);
      lastGroup = f.group;
    }
    const conv = !!(f.fromSteps && spi > 0);   // show this field in inches/sec instead of steps
    const group = el("div", "input-group");
    group.appendChild(el("label", null, f.label || f.key));
    const rowEl = el("div", "input-row");
    const inp = document.createElement("input");
    inp.type = "number"; inp.inputMode = "decimal";
    inp.dataset.key = f.key;
    inp.dataset.type = f.type || "float";
    if (conv) inp.dataset.spi = String(spi);   // marks a steps↔inches field for commit/reconcile
    if (f.min != null) inp.min = conv ? round2(f.min / spi) : f.min;
    if (f.max != null) inp.max = conv ? round2(f.max / spi) : f.max;
    inp.step = conv ? "any" : (f.step != null ? f.step : "any");
    const dval = conv ? round2(f.value / spi) : f.value;
    inp.dataset.orig = String(dval);
    inp.value = dval;
    if (!s.online) inp.disabled = true;
    const stat = el("span", "fstat");
    inp._stat = stat;
    inp.addEventListener("blur", () => commitField(s, inp, stat, f));
    inp.addEventListener("keydown", e => { if (e.key === "Enter") inp.blur(); });
    rowEl.appendChild(inp); rowEl.appendChild(stat);
    group.appendChild(rowEl);
    (inner || s.fields).appendChild(group);
  });
  scheduleRelayout();   // config fields populated — page height changed
}

// True while any field on this machine is waiting for a deferred value to apply at idle.
function hasPending(s) {
  if (!s || !s.configLoaded) return false;
  return Array.prototype.some.call(s.fields.querySelectorAll("input"), i => i._pending);
}

// Periodically re-pull /api/config so displayed values track the device after a reboot,
// another client's edit, or a deferred value applying at next IDLE. Never touches the field
// being edited or one with an uncommitted local change.
async function reconcileConfig(s) {
  if (!s.configLoaded || !s.online) return;
  let r = null;
  try { r = await fetchJSON(s.m.base + "/api/config"); } catch (e) {}
  if (!r || !r.ok || !r.json || !Array.isArray(r.json.fields)) return;
  const spi = Number(r.json.stepsPerInch) || 0;
  r.json.fields.forEach(f => {
    const inp = s.fields.querySelector('input[data-key="' + f.key + '"]');
    if (!inp || inp === document.activeElement) return;
    // Compare in the SAME unit the field is displayed in (inches/sec for fromSteps).
    const live = String((f.fromSteps && spi > 0) ? round2(f.value / spi) : f.value);
    if (inp._pending) {  // a deferred save has landed once the device reports the staged value
      if (inp.dataset.orig === live) { inp._pending = false; inp.value = live; setStat(inp._stat, "ok", "✓", "applied at idle", true); }
      return;
    }
    if (String(inp.value).trim() !== inp.dataset.orig) return;  // uncommitted local edit — leave it
    if (inp.dataset.orig !== live) { inp.value = live; inp.dataset.orig = live; }
  });
}

// Render an inline per-field status icon. kind: "spin" | "ok" | "warn" | "err".
function setStat(stat, kind, glyph, title, fade) {
  stat.className = "fstat" + (kind && kind !== "spin" ? " " + kind : "");
  stat.title = title || "";
  stat.innerHTML = "";
  if (kind === "spin") { stat.appendChild(el("span", "spin")); return; }
  if (!glyph) return;
  const g = el("span", fade ? "fade" : null, glyph);
  if (fade) g.addEventListener("animationend", () => { if (stat.firstChild === g) { stat.innerHTML = ""; stat.title = ""; } });
  stat.appendChild(g);
}

// Save-on-blur: POST just this field if it changed and is in range.
async function commitField(s, inp, stat, f) {
  if (inp.disabled) { inp.value = inp.dataset.orig; inp.classList.remove("err"); setStat(stat); return; }
  const raw = inp.value.trim();
  const spi = Number(inp.dataset.spi) || 0;   // >0 → field shown in inches/sec, stored in steps
  const conv = spi > 0;
  const isInt = f.type === "int";
  const val = parseFloat(raw);                 // value in DISPLAY units (inches/sec when conv)

  if (raw === "" || isNaN(val)) { inp.value = inp.dataset.orig; inp.classList.remove("err"); setStat(stat); return; }
  // Whole-number rule applies only to int fields shown in their native unit; a
  // fromSteps field shows fractional in/s and is rounded to whole steps on send.
  if (isInt && !conv && !Number.isInteger(val)) { inp.classList.add("err"); setStat(stat, "err", "✕", "must be a whole number"); return; }
  if (String(val) === inp.dataset.orig) { inp.classList.remove("err"); setStat(stat); return; }
  const dmin = inp.min !== "" ? parseFloat(inp.min) : null;
  const dmax = inp.max !== "" ? parseFloat(inp.max) : null;
  if ((dmin != null && val < dmin) || (dmax != null && val > dmax)) {
    inp.classList.add("err");
    setStat(stat, "err", "✕", "must be " + inp.min + "–" + inp.max);
    return;
  }
  inp.classList.remove("err");

  // Convert inches/sec back to steps for the wire, clamping to the device's
  // integer step bounds so a value typed at the rounded display limit can't
  // round just past the real min/max and get rejected.
  const sendVal = conv ? Math.max(f.min, Math.min(f.max, Math.round(val * spi))) : val;

  const gen = inp._gen = (inp._gen || 0) + 1;
  setStat(stat, "spin");
  const payload = {}; payload[f.key] = sendVal;
  let r = null;
  try {
    r = await fetchJSON(s.m.base + "/api/config", {
      method:"POST", headers:{ "Content-Type":"text/plain" }, body:JSON.stringify(payload),
    });
  } catch (e) {}
  if (inp._gen !== gen) return;  // superseded by a newer edit
  const j = (r && r.json) || {};
  if (r && r.ok && j.ok) {
    // Store orig/value in the SAME rounded inches unit reconcile compares against,
    // so a >2-decimal entry can't leave _pending stuck or snap the field back.
    const disp = conv ? round2(sendVal / spi) : val;
    inp.dataset.orig = String(disp);
    inp.value = String(disp);
    if (j.deferred) { inp._pending = true; setStat(stat, "warn", "⏱", "pending — applies at next idle"); }
    else { inp._pending = false; setStat(stat, "ok", "✓", "saved", true); }
  } else if (!r) {
    inp.value = inp.dataset.orig;  // not saved → restore device value so reconcile can re-sync
    setStat(stat, "warn", "⚠", "machine unreachable — not saved");
  } else {
    setStat(stat, "err", "✕", j.message || "save failed");
  }
}

// Stage 1 mode (3 Inch / Minis) — optimistic toggle, posts the reserved __mode__ key.
function updateModeUI(s, mode, animate) {
  if (!s.modeToggle) return;
  const prev = s.mode;
  s.mode = mode;
  s.modeOpt0.classList.toggle("active", mode === 0);
  s.modeOpt1.classList.toggle("active", mode === 1);
  s.modeToggle.classList.remove("mode-0", "mode-1");
  s.modeToggle.classList.add("mode-" + mode);
  if (animate && prev != null && prev !== mode && s.modeBubble) {
    s.modeBubble.classList.remove("squish");
    void s.modeBubble.offsetWidth;  // reflow → restart animation
    s.modeBubble.classList.add("squish");
    setTimeout(() => s.modeBubble.classList.remove("squish"), 440);
  }
}

async function setMachineMode(s, mode) {
  if (!s.online) { showCfgStatus(s, "machine offline", "err"); return; }
  if (s.mode === mode) return;
  const prev = s.mode;
  s._modePending = mode;        // guard: poll-driven status must not snap the toggle back while in flight
  updateModeUI(s, mode, true);  // optimistic
  let r = null;
  try {
    r = await fetchJSON(s.m.base + "/api/config", {
      method:"POST", headers:{ "Content-Type":"text/plain" }, body:JSON.stringify({ __mode__: mode }),
    });
  } catch (e) {}
  s._modePending = null;
  const j = (r && r.json) || {};
  if (r && r.ok && j.ok) {
    updateModeUI(s, mode, false);  // lock in the applied mode
    showCfgStatus(s, "Switched to " + (mode === 0 ? "3 Inch" : "Minis"), "ok");
    s.configLoaded = false; loadConfig(s);  // mode swap changes field values — re-pull
  } else {
    if (prev != null) updateModeUI(s, prev, true);  // revert
    showCfgStatus(s, (j.message || "mode change failed"), "err");
  }
}

function showCfgStatus(s, msg, kind) {
  s.cfgStatus.textContent = msg;
  s.cfgStatus.style.color = kind === "err" ? "var(--accent-danger)" : "var(--accent-success)";
  clearTimeout(s._cfgTimer);
  s._cfgTimer = setTimeout(() => { s.cfgStatus.textContent = ""; }, 3000);
}

// Backup All — pull every machine's /api/config and bundle into one JSON file.
function downloadJSON(obj, filename) {
  const blob = new Blob([JSON.stringify(obj, null, 2)], { type:"application/json" });
  const url = URL.createObjectURL(blob);
  const a = document.createElement("a"); a.href = url; a.download = filename;
  document.body.appendChild(a); a.click(); document.body.removeChild(a);
  URL.revokeObjectURL(url);
}

async function backupAll() {
  const btn = document.getElementById("backupBtn");
  const label = btn.textContent;
  btn.disabled = true; btn.textContent = "Backing up…";
  const out = { type:"production_line_config_backup", exportedAt:new Date().toISOString(), machines:{} };
  let okCount = 0;
  await Promise.all(MACHINES.map(async m => {
    const s = ST[m.id];
    let r = null;
    try { r = await fetchJSON(m.base + "/api/config"); } catch (e) {}
    const ok = !!(r && r.ok && r.json && Array.isArray(r.json.fields));
    const entry = { name:m.name, base:m.base || "same origin", online:ok, fields:{} };
    if (ok) { okCount++; r.json.fields.forEach(f => { entry.fields[f.key] = f.value; }); }
    if (m.hasMode && s && s.mode != null) entry.mode = s.mode;  // Stage 1 3 Inch / Minis
    out.machines[m.id] = entry;
  }));
  const ts = new Date().toISOString().slice(0, 19).replace(/[:T]/g, "-");
  downloadJSON(out, "production-line-config-" + ts + ".json");
  btn.textContent = okCount === MACHINES.length ? "Saved ✓" : ("Saved " + okCount + "/" + MACHINES.length);
  setTimeout(() => { btn.textContent = label; btn.disabled = false; }, 2200);
}

// Header summary
function updateSummary() {
  const total = MACHINES.length;
  const online = MACHINES.filter(m => ST[m.id].online).length;
  const badge = document.getElementById("summaryBadge");
  badge.className = "status-badge " + (online === total ? "connected" : online > 0 ? "partial" : "disconnected");
  document.getElementById("sum-txt").textContent = online + " / " + total + " online";
}

// Boot + poll loop
const tabsEl = document.getElementById("tabs");
const stage = document.getElementById("stage");
MACHINES.forEach(m => {
  const p = buildPanel(m);
  stage.appendChild(p);
  const tab = el("button", "tab");
  const tdot = el("span", "tdot");
  tab.appendChild(tdot); tab.appendChild(el("span", null, m.name));
  tab.onclick = () => setActive(m.id, "smooth");   // tab tap glides to that machine's page
  tabsEl.appendChild(tab);
  ST[m.id].tab = tab; ST[m.id].tabDot = tdot;
});
stage.addEventListener("scroll", onStageScroll, { passive: true });   // swipe drives the active page
window.addEventListener("resize", () => { setActive(active, "instant"); scheduleRelayout(); });
if (document.fonts && document.fonts.ready) document.fonts.ready.then(scheduleRelayout);  // re-measure once webfont lands
setActive(active, "instant");   // land on the default page, snapped, no animation
scheduleRelayout();
document.getElementById("backupBtn").onclick = backupAll;
async function tick() {
  await Promise.all(MACHINES.map(poll));
  updateSummary();
  // Reconcile on the periodic cadence, plus every poll for any machine with a pending
  // deferred change so its ✓ lands promptly once the machine reaches idle.
  const periodic = pollCount++ % RECONCILE_EVERY === RECONCILE_EVERY - 1;
  await Promise.all(MACHINES.map(m => {
    const s = ST[m.id];
    if (periodic || hasPending(s)) return reconcileConfig(s);
  }));
}
tick();
setInterval(tick, POLL_MS);
// Paint the uptime locally off the animation frame so the second flips exactly when it
// should — one clean tick per second, decoupled from poll cadence/latency. Only writes the
// DOM when the formatted text actually changes (no per-frame churn).
function paintUptime() {
  const now = performance.now();
  MACHINES.forEach(m => {
    const s = ST[m.id];
    if (!s || !s.online || s.uptimeEpoch == null) return;
    const txt = fmtUptime(now - s.uptimeEpoch);
    if (txt !== s.uptimeText) { s.uptimeText = txt; s.uptime.textContent = txt; }
  });
  requestAnimationFrame(paintUptime);
}
requestAnimationFrame(paintUptime);
</script>
</body>
</html>)rawliteral";
