// reRoMouse Telemetry Dashboard
//   Phase 5: 数値表示 MVP・データ駆動(受信 JSON をセクション化して自動表示)
//   Phase 6b: 自前 canvas チャート(数値クリックでピン留め)＋自己位置 2D
//
// 依存ゼロ(フレームワーク/外部ライブラリ無し)。グラフ/2D は素の Canvas2D で描画する。
(function () {
  'use strict';

  // --- 設定 ---
  const WINDOW_MS = 10000; // グラフの時間窓(直近 N ms)
  const MAZE_N = 16;       // 迷路セル数(一辺)
  const CELL_MM = 180;     // 1 セルの実寸 [mm](pose は [m] 想定: m*1000/CELL_MM = セル)
  const TRAJ_MAX = 4000;   // 軌跡の保持点数(メモリ上限)
  const COL_RAW = '#3498db';
  const COL_CORR = '#e67e22';
  const COL_CUR = '#2ecc71';

  const $ = (sel) => document.querySelector(sel);
  const elStatus = $('#status');
  const elFps = $('#fps');
  const elBatt = $('#batt');
  const elVer = $('#ver');
  const cards = $('#cards');
  const graphsEl = $('#graphs');

  // セクションキー -> 表示名(未知キーは生キーをそのまま使う=自動追従)
  const TITLES = {
    info: 'デバイス情報', hw: 'ハードウェア', sens: 'センサ', pose: '自己位置',
    motion: '走行値', stat: 'ステータス', misc: 'その他',
  };

  function fmt(v) {
    if (typeof v === 'number') {
      return Number.isInteger(v) ? String(v) : (+v.toFixed(4)).toString();
    }
    if (typeof v === 'boolean') return v ? 'true' : 'false';
    return String(v);
  }

  // ネストオブジェクトを [dottedKey, 生値] の葉ペアへ平坦化(配列は添字キー)。
  function flatten(obj, prefix, out) {
    for (const [k, v] of Object.entries(obj)) {
      const key = prefix ? prefix + '.' + k : k;
      if (v !== null && typeof v === 'object') flatten(v, key, out);
      else out.push([key, v]);
    }
    return out;
  }

  // dotted path で値を取り出す("gyro.deg" / "tasks.0.us" 等)。無ければ undefined。
  function getPath(obj, path) {
    if (obj == null) return undefined;
    let cur = obj;
    for (const part of path.split('.')) {
      if (cur == null || typeof cur !== 'object') return undefined;
      cur = cur[part];
    }
    return cur;
  }

  // ============ データ駆動の数値カード ============
  // セクションキー -> {card, tbody, rowMap:Map(dottedKey->{tr,cell})}
  const cache = new Map();

  function ensureCard(key) {
    let c = cache.get(key);
    if (c) return c;
    const card = document.createElement('div');
    card.className = 'card';
    const h = document.createElement('h2');
    h.append(TITLES[key] || key, ' ');
    const code = document.createElement('code');
    code.textContent = key;
    h.appendChild(code);
    const table = document.createElement('table');
    const tbody = document.createElement('tbody');
    table.appendChild(tbody);
    card.appendChild(h);
    card.appendChild(table);
    cards.appendChild(card);
    c = { card, tbody, rowMap: new Map() };
    cache.set(key, c);
    return c;
  }

  // 1 セクションを描画。既存行は値だけ更新し、消えた項目は淡色化。
  // 数値行はクリックでグラフへピン留め可能にする(info は静的なので対象外)。
  function renderSection(key, value) {
    const c = ensureCard(key);
    const rows = (value !== null && typeof value === 'object')
      ? flatten(value, '', [])
      : [[key, value]];
    const seen = new Set();
    for (const [k, raw] of rows) {
      seen.add(k);
      const text = fmt(raw);
      let row = c.rowMap.get(k);
      if (!row) {
        const tr = document.createElement('tr');
        const tdk = document.createElement('td');
        tdk.className = 'k';
        tdk.textContent = k;
        const tdv = document.createElement('td');
        tdv.className = 'v';
        tr.append(tdk, tdv);
        c.tbody.appendChild(tr);
        row = { tr, cell: tdv };
        c.rowMap.set(k, row);
        // 数値かつストリーミングセクションのみピン留め可能に。
        if (typeof raw === 'number' && key !== 'info') {
          const id = key + '.' + k;
          tr.classList.add('pinnable');
          tr.addEventListener('click', () => togglePin(id));
        }
      }
      if (row.cell.textContent !== text) row.cell.textContent = text;
      row.tr.style.opacity = '';
    }
    for (const [k, row] of c.rowMap) {
      if (!seen.has(k)) row.tr.style.opacity = '0.35';
    }
  }

  // ============ ピン留めグラフ(自前 canvas) ============
  // id -> {buf:[{t,v}], canvas, ctx, cssW, cssH, minEl, vEl, maxEl}
  const pinned = new Map();

  function setupCanvas(canvas, cssW, cssH) {
    const dpr = window.devicePixelRatio || 1;
    canvas.width = Math.round(cssW * dpr);
    canvas.height = Math.round(cssH * dpr);
    const ctx = canvas.getContext('2d');
    ctx.scale(dpr, dpr); // 以降は CSS px 座標で描ける
    return ctx;
  }

  function markPinned(id, on) {
    // 対応する行に pinned クラスを反映(セクション.キー で逆引き)。
    const dot = id.indexOf('.');
    const c = cache.get(id.slice(0, dot));
    if (!c) return;
    const row = c.rowMap.get(id.slice(dot + 1));
    if (row) row.tr.classList.toggle('pinned', on);
  }

  function togglePin(id) {
    if (pinned.has(id)) {
      pinned.get(id).card.remove();
      pinned.delete(id);
      markPinned(id, false);
      return;
    }
    const card = document.createElement('div');
    card.className = 'card chart';
    const h = document.createElement('h2');
    const label = document.createElement('span');
    label.textContent = id;
    const sp = document.createElement('span');
    sp.className = 'sp';
    const x = document.createElement('button');
    x.className = 'btn-x';
    x.textContent = '✕';
    x.title = 'ピン留め解除';
    x.addEventListener('click', () => togglePin(id));
    h.append(label, sp, x);
    const canvas = document.createElement('canvas');
    const meta = document.createElement('div');
    meta.className = 'meta';
    const minEl = document.createElement('span');
    const vEl = document.createElement('span');
    const maxEl = document.createElement('span');
    meta.append(minEl, vEl, maxEl);
    card.append(h, canvas, meta);
    card.dataset.card = id;
    graphsEl.appendChild(card);
    const cssW = canvas.clientWidth || 280;
    const ctx = setupCanvas(canvas, cssW, 120);
    pinned.set(id, { buf: [], card, canvas, ctx, cssW, cssH: 120, minEl, vEl, maxEl });
    markPinned(id, true);
  }

  function drawChart(g) {
    const { ctx, cssW: W, cssH: H, buf } = g;
    ctx.clearRect(0, 0, W, H);
    if (buf.length === 0) return;
    const now = performance.now();
    const t0 = now - WINDOW_MS;
    let mn = Infinity, mx = -Infinity;
    for (const p of buf) { if (p.v < mn) mn = p.v; if (p.v > mx) mx = p.v; }
    if (mn === mx) { mn -= 1; mx += 1; } // 平坦時に潰れないように
    const pad = 4;
    const xOf = (t) => ((t - t0) / WINDOW_MS) * (W - 2 * pad) + pad;
    const yOf = (v) => H - pad - ((v - mn) / (mx - mn)) * (H - 2 * pad);
    // 0 基準線
    if (mn < 0 && mx > 0) {
      ctx.strokeStyle = '#8884';
      ctx.lineWidth = 1;
      ctx.beginPath();
      ctx.moveTo(pad, yOf(0));
      ctx.lineTo(W - pad, yOf(0));
      ctx.stroke();
    }
    ctx.strokeStyle = COL_RAW;
    ctx.lineWidth = 1.5;
    ctx.beginPath();
    let started = false;
    for (const p of buf) {
      const x = xOf(p.t), y = yOf(p.v);
      if (!started) { ctx.moveTo(x, y); started = true; }
      else ctx.lineTo(x, y);
    }
    ctx.stroke();
    const last = buf[buf.length - 1].v;
    g.minEl.textContent = (+mn.toFixed(3)).toString();
    g.vEl.textContent = (+last.toFixed(3)).toString();
    g.maxEl.textContent = (+mx.toFixed(3)).toString();
  }

  // 全ピングラフへ現フレームの値をサンプリングする。
  function sampleGraphs(data, t) {
    for (const [id, g] of pinned) {
      const dot = id.indexOf('.');
      const section = id.slice(0, dot);
      const rest = id.slice(dot + 1);
      const v = section === 'misc' ? data[rest] : getPath(data[section], rest);
      if (typeof v === 'number') {
        g.buf.push({ t, v });
        const cut = t - WINDOW_MS;
        while (g.buf.length && g.buf[0].t < cut) g.buf.shift();
      }
    }
  }

  // ============ 自己位置 2D ============
  const poseCanvas = $('#poseCanvas');
  const poseInfo = $('#pose-info');
  const POSE_W = 320, POSE_H = 320;
  const poseCtx = setupCanvas(poseCanvas, POSE_W, POSE_H);
  const rawTraj = [];
  const corrTraj = [];
  let poseCur = null;

  // pose 値([m] 想定) -> 迷路セル座標(0..MAZE_N)。
  const toCell = (m) => (m * 1000) / CELL_MM;
  // セル座標 -> canvas px(y は上向き→下向きへ反転)。
  function cellToPx(cx, cy) {
    const pad = 12;
    const sz = Math.min(POSE_W, POSE_H) - 2 * pad;
    return [pad + (cx / MAZE_N) * sz, POSE_H - pad - (cy / MAZE_N) * sz];
  }

  function drawTraj(pts, color) {
    if (pts.length < 2) return;
    poseCtx.strokeStyle = color;
    poseCtx.lineWidth = 1.5;
    poseCtx.beginPath();
    let started = false;
    for (const p of pts) {
      const [x, y] = cellToPx(toCell(p.x), toCell(p.y));
      if (!started) { poseCtx.moveTo(x, y); started = true; }
      else poseCtx.lineTo(x, y);
    }
    poseCtx.stroke();
  }

  function drawPose() {
    const ctx = poseCtx;
    ctx.clearRect(0, 0, POSE_W, POSE_H);
    const pad = 12;
    const sz = Math.min(POSE_W, POSE_H) - 2 * pad;
    // グリッド
    ctx.strokeStyle = '#8883';
    ctx.lineWidth = 1;
    ctx.beginPath();
    for (let i = 0; i <= MAZE_N; i++) {
      const p = pad + (i / MAZE_N) * sz;
      ctx.moveTo(pad, p); ctx.lineTo(pad + sz, p);
      ctx.moveTo(p, pad); ctx.lineTo(p, pad + sz);
    }
    ctx.stroke();
    // 現在セルのハイライト + 方位
    if (poseCur && poseCur.cell) {
      const { x: cx, y: cy, dir } = poseCur.cell;
      if (Number.isFinite(cx) && Number.isFinite(cy)) {
        const cw = sz / MAZE_N;
        const [px, py] = cellToPx(cx, cy); // セル(cx,cy)の左下
        ctx.fillStyle = COL_CUR + '44';
        ctx.fillRect(px, py - cw, cw, cw);
        // 方位矢印(dir: 0=N,1=E,2=S,3=W を優先。無ければ θ)
        const ccx = px + cw / 2, ccy = py - cw / 2;
        let ang;
        if (typeof dir === 'number') ang = [Math.PI / 2, 0, -Math.PI / 2, Math.PI][dir & 3];
        else ang = (poseCur.th || 0);
        const r = cw * 0.4;
        ctx.strokeStyle = COL_CUR;
        ctx.lineWidth = 2;
        ctx.beginPath();
        ctx.moveTo(ccx, ccy);
        ctx.lineTo(ccx + r * Math.cos(ang), ccy - r * Math.sin(ang));
        ctx.stroke();
      }
    }
    // 軌跡(生 / 補正後)
    drawTraj(rawTraj, COL_RAW);
    drawTraj(corrTraj, COL_CORR);
  }

  function updatePose(data) {
    const p = data.pose;
    if (!p) return;
    if (typeof p.x === 'number' && typeof p.y === 'number') {
      rawTraj.push({ x: p.x, y: p.y });
      if (rawTraj.length > TRAJ_MAX) rawTraj.shift();
    }
    if (typeof p.xc === 'number' && typeof p.yc === 'number') {
      corrTraj.push({ x: p.xc, y: p.yc });
      if (corrTraj.length > TRAJ_MAX) corrTraj.shift();
    }
    poseCur = { th: p.th, cell: (data.stat && data.stat.cell) || null };
    const f = (v) => (typeof v === 'number' ? (+v.toFixed(3)).toString() : '—');
    const cell = poseCur.cell || {};
    poseInfo.textContent =
      `x=${f(p.x)} y=${f(p.y)} θ=${f(p.th)} | cell(${cell.x ?? '—'},${cell.y ?? '—'}) dir=${cell.dir ?? '—'}`;
  }

  // ============ フレーム描画 ============
  function render(data) {
    if (data.sens && typeof data.sens.batt === 'number') {
      elBatt.textContent = data.sens.batt.toFixed(2) + ' V';
    }
    const scalars = {};
    for (const [key, value] of Object.entries(data)) {
      if (value !== null && typeof value === 'object') renderSection(key, value);
      else scalars[key] = value;
    }
    if (Object.keys(scalars).length) renderSection('misc', scalars);
    sampleGraphs(data, performance.now());
    updatePose(data);
  }

  // rAF で描画を間引き(受信は 20Hz でも描画は画面リフレッシュに同期)。
  function drawLoop() {
    for (const g of pinned.values()) drawChart(g);
    drawPose();
    requestAnimationFrame(drawLoop);
  }
  requestAnimationFrame(drawLoop);

  // --- 受信レート(fps) ---
  let frames = 0, lastT = performance.now();
  setInterval(() => {
    const now = performance.now();
    elFps.textContent = (frames * 1000 / (now - lastT)).toFixed(1);
    frames = 0;
    lastT = now;
  }, 1000);

  // --- デバイス静的情報(1 回取得) ---
  fetch('/api/info')
    .then((r) => r.json())
    .then((j) => renderSection('info', j))
    .catch(() => {});

  // --- レート / 一時停止 操作(既存 WS cmd backend へ送信) ---
  let sock = null;     // 現在の WS(送信用)
  let paused = false;  // UI 側の一時停止状態
  const elRate = $('#rate');
  const elRateV = $('#ratev');
  const elPause = $('#pause');
  function sendCmd(obj) {
    if (sock && sock.readyState === WebSocket.OPEN) sock.send(JSON.stringify(obj));
  }
  elRate.addEventListener('input', () => {
    elRateV.textContent = elRate.value;
    sendCmd({ rate: +elRate.value });
  });
  elPause.addEventListener('click', () => {
    paused = !paused;
    elPause.textContent = paused ? '▶ 再開' : '⏸ 一時停止';
    sendCmd({ pause: paused });
  });

  // --- WS ライブストリーム(指数バックオフ再接続) ---
  function setStatus(text, bg, fg) {
    elStatus.textContent = text;
    elStatus.style.background = bg;
    elStatus.style.color = fg;
  }

  let backoff = 500;
  function connect() {
    const ws = new WebSocket('ws://' + location.host + '/ws');
    ws.onopen = () => {
      sock = ws;
      setStatus('LIVE', '#2ecc71', '#053');
      backoff = 500;
      // UI の現在状態をサーバへ反映(再接続時の同期)。
      sendCmd({ rate: +elRate.value });
      if (paused) sendCmd({ pause: true });
    };
    ws.onmessage = (ev) => {
      frames++;
      let msg;
      try { msg = JSON.parse(ev.data); } catch (e) { return; }
      if (msg.v != null) elVer.textContent = 'schema v' + msg.v;
      render(msg.data || msg);
    };
    ws.onclose = () => {
      if (sock === ws) sock = null;
      setStatus('OFF', '#e74c3c', '#fff');
      setTimeout(connect, backoff);
      backoff = Math.min(backoff * 2, 5000);
    };
    ws.onerror = () => ws.close();
  }
  connect();
})();
