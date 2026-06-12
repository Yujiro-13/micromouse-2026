// reRoMouse Telemetry Dashboard (Phase 5: 数値表示 MVP・データ駆動)
//
// 受信 JSON(WS /ws の envelope data, および GET /api/info)を再帰走査し、
// セクション毎に key->value 表を自動生成する。サーバが項目を増減しても
// 画面コードを変えずに自動追従する(FR-9)。グラフ等は Phase 6 で追加。
(function () {
  'use strict';

  const $ = (sel) => document.querySelector(sel);
  const elStatus = $('#status');
  const elFps = $('#fps');
  const elBatt = $('#batt');
  const elVer = $('#ver');
  const cards = $('#cards');

  // セクションキー -> 表示名(未知キーは生キーをそのまま使う=自動追従)
  const TITLES = {
    info: 'デバイス情報',
    hw: 'ハードウェア',
    sens: 'センサ',
    pose: '自己位置',
    motion: '走行値',
    stat: 'ステータス',
    misc: 'その他',
  };

  function fmt(v) {
    if (typeof v === 'number') {
      return Number.isInteger(v) ? String(v) : (+v.toFixed(4)).toString();
    }
    if (typeof v === 'boolean') return v ? 'true' : 'false';
    return String(v);
  }

  // ネストオブジェクトを [dottedKey, 表示文字列] の葉ペアへ平坦化。
  function flatten(obj, prefix, out) {
    for (const [k, v] of Object.entries(obj)) {
      const key = prefix ? prefix + '.' + k : k;
      if (v !== null && typeof v === 'object') flatten(v, key, out);
      else out.push([key, fmt(v)]);
    }
    return out;
  }

  // セクションキー -> {card, tbody, rowMap:Map(dottedKey->valueCell)}
  const cache = new Map();

  function ensureCard(key) {
    let c = cache.get(key);
    if (c) return c;
    const card = document.createElement('div');
    card.className = 'card';
    const h = document.createElement('h2');
    h.textContent = (TITLES[key] || key) + ' ';
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

  // 1 セクションを描画。既存行は値だけ更新し、消えた項目は淡色化(グレースフル劣化)。
  function renderSection(key, value) {
    const c = ensureCard(key);
    const rows = (value !== null && typeof value === 'object')
      ? flatten(value, '', [])
      : [[key, fmt(value)]];
    const seen = new Set();
    for (const [k, v] of rows) {
      seen.add(k);
      let cell = c.rowMap.get(k);
      if (!cell) {
        const tr = document.createElement('tr');
        const tdk = document.createElement('td');
        tdk.className = 'k';
        tdk.textContent = k;
        const tdv = document.createElement('td');
        tdv.className = 'v';
        tr.appendChild(tdk);
        tr.appendChild(tdv);
        c.tbody.appendChild(tr);
        cell = tdv;
        c.rowMap.set(k, cell);
      }
      if (cell.textContent !== v) cell.textContent = v;
      cell.parentElement.style.opacity = '';
    }
    for (const [k, cell] of c.rowMap) {
      if (!seen.has(k)) cell.parentElement.style.opacity = '0.35';
    }
  }

  // テレメトリ 1 フレームの data を描画。オブジェクトはセクション、スカラは misc にまとめる。
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
  }

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

  // --- WS ライブストリーム(指数バックオフ再接続) ---
  function setStatus(text, bg, fg) {
    elStatus.textContent = text;
    elStatus.style.background = bg;
    elStatus.style.color = fg;
  }

  let backoff = 500;
  function connect() {
    const ws = new WebSocket('ws://' + location.host + '/ws');
    ws.onopen = () => { setStatus('LIVE', '#2ecc71', '#053'); backoff = 500; };
    ws.onmessage = (ev) => {
      frames++;
      let msg;
      try { msg = JSON.parse(ev.data); } catch (e) { return; }
      if (msg.v != null) elVer.textContent = 'schema v' + msg.v;
      render(msg.data || msg);
    };
    ws.onclose = () => {
      setStatus('OFF', '#e74c3c', '#fff');
      setTimeout(connect, backoff);
      backoff = Math.min(backoff * 2, 5000);
    };
    ws.onerror = () => ws.close();
  }
  connect();
})();
