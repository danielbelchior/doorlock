#pragma once

#include <Arduino.h>

// Single-page web UI (Portuguese). Talks to the JSON API in web_ui.cpp.
// Layout: one column on phones, two columns from 768 px (tablets and up).
static const char INDEX_HTML[] PROGMEM = R"rawliteral(<!DOCTYPE html>
<html lang="pt-BR">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1,viewport-fit=cover">
<meta name="color-scheme" content="light dark">
<title>Teclado ESP32</title>
<style>
:root{--bg:#f2f4f7;--card:#fff;--text:#1d2939;--muted:#667085;--line:#e4e7ec;--accent:#0e7c66;--on-accent:#fff;--danger:#c4320a;--ok:#d1fadf;--on-ok:#05603a;--radius:14px}
@media (prefers-color-scheme:dark){:root{--bg:#0f1418;--card:#182026;--text:#e7ecf0;--muted:#98a2b3;--line:#2a343c;--accent:#33c3a0;--on-accent:#04221b;--danger:#f97066;--ok:#0c3b2a;--on-ok:#75e0a7}}
*{box-sizing:border-box}
html{-webkit-text-size-adjust:100%}
body{margin:0;font:16px/1.5 system-ui,-apple-system,"Segoe UI",Roboto,sans-serif;background:var(--bg);color:var(--text)}
[hidden]{display:none!important}
a{color:inherit;font-weight:600}
code{font-family:ui-monospace,Menlo,Consolas,monospace;font-size:.9em}
.wrap{max-width:1120px;margin:0 auto;padding-left:max(16px,env(safe-area-inset-left));padding-right:max(16px,env(safe-area-inset-right))}
header{position:sticky;top:0;z-index:10;background:var(--card);border-bottom:1px solid var(--line);padding-top:env(safe-area-inset-top)}
.top{display:flex;align-items:center;gap:12px;padding-top:12px;padding-bottom:4px}
h1{flex:1;margin:0;font-size:1.25rem}
.pill{display:inline-flex;align-items:center;gap:8px;max-width:50%;padding:4px 12px;border-radius:999px;background:var(--line);color:var(--muted);font-size:.85rem;white-space:nowrap;overflow:hidden;text-overflow:ellipsis}
.pill::before{content:"";flex:none;width:8px;height:8px;border-radius:50%;background:currentColor}
.pill.on{background:var(--ok);color:var(--on-ok)}
nav{display:flex;gap:4px;overflow-x:auto}
nav button{flex:1 1 0;min-height:48px;padding:10px 16px;border:0;border-bottom:3px solid transparent;border-radius:0;background:none;color:var(--muted);font:inherit;font-weight:600;cursor:pointer}
nav button.active{color:var(--accent);border-bottom-color:var(--accent)}
main{padding-top:20px;padding-bottom:calc(40px + env(safe-area-inset-bottom))}
.grid{display:grid;gap:20px;align-items:start}
.col{display:grid;gap:20px}
.card{background:var(--card);border:1px solid var(--line);border-radius:var(--radius);padding:20px}
h2{display:flex;align-items:center;justify-content:space-between;gap:8px;margin:0 0 16px;font-size:1.1rem}
.count{color:var(--muted);font-size:.85rem;font-weight:500}
label{display:block;margin-bottom:14px;font-size:.95rem;font-weight:600}
input{display:block;width:100%;min-height:48px;margin-top:6px;padding:10px 14px;border:1px solid var(--line);border-radius:10px;background:var(--bg);color:var(--text);font:inherit;font-size:16px}
input[type=file]{padding:10px}
label.check{display:flex;align-items:center;gap:12px;min-height:48px;cursor:pointer}
label.check input{flex:none;width:24px;height:24px;min-height:0;margin:0;padding:0;accent-color:var(--accent)}
input:focus{outline:2px solid var(--accent);outline-offset:1px}
button{min-height:44px;padding:10px 18px;border:1px solid var(--line);border-radius:10px;background:var(--card);color:var(--text);font:inherit;font-weight:600;cursor:pointer;touch-action:manipulation}
button:disabled{opacity:.55;cursor:default}
button.primary{background:var(--accent);border-color:var(--accent);color:var(--on-accent)}
button.danger{color:var(--danger)}
.block{display:block;width:100%}
.row{display:flex;flex-wrap:wrap;gap:10px}
.row>button{flex:1 1 auto}
.hint{margin:-6px 0 14px;color:var(--muted);font-size:.9rem}
.hint.lead{margin-top:0}
.users,.scan{display:grid;gap:10px;margin:0;padding:0;list-style:none}
.user{display:flex;flex-wrap:wrap;align-items:center;gap:12px;min-width:0;padding:12px 14px;border:1px solid var(--line);border-radius:12px}
.user.editing{border-color:var(--accent);box-shadow:0 0 0 1px var(--accent)}
.who{flex:1 1 160px;min-width:0}
.who strong{display:block;overflow:hidden;text-overflow:ellipsis;white-space:nowrap}
.pin{color:var(--muted);font-family:ui-monospace,Menlo,Consolas,monospace;letter-spacing:.15em}
.tag{display:inline-block;margin-left:10px;padding:0 8px;border-radius:999px;background:var(--line);color:var(--muted);font-size:.8rem}
.tag.on{background:var(--ok);color:var(--on-ok)}
.actions{display:flex;flex-wrap:wrap;gap:8px}
.actions button{padding:8px 14px}
.empty{padding:24px;color:var(--muted);text-align:center}
.scan{gap:6px;margin-bottom:14px}
.scan button{display:flex;justify-content:space-between;gap:12px;width:100%;font-weight:500;text-align:left}
dl.kv{display:grid;grid-template-columns:auto 1fr;gap:10px 16px;margin:0}
dl.kv dt{color:var(--muted)}
dl.kv dd{margin:0;font-weight:600;overflow-wrap:anywhere}
progress{display:block;width:100%;height:12px;margin:0 0 14px}
#toast{position:fixed;left:50%;bottom:calc(20px + env(safe-area-inset-bottom));z-index:20;max-width:min(560px,calc(100% - 32px));padding:14px 20px;border-radius:12px;background:var(--text);color:var(--bg);font-weight:600;box-shadow:0 8px 24px rgba(0,0,0,.25);transform:translate(-50%,calc(100% + 40px));transition:transform .25s}
#toast.show{transform:translate(-50%,0)}
#toast.bad{background:var(--danger);color:#fff}
@media (min-width:768px){
 h1{font-size:1.4rem}
 nav button{flex:0 0 auto;min-width:150px}
 main{padding-top:28px}
 .grid.two{grid-template-columns:minmax(0,3fr) minmax(300px,2fr)}
 .card{padding:24px}
}
@media (min-width:1024px){.users{grid-template-columns:repeat(auto-fill,minmax(340px,1fr))}}
</style>
</head>
<body>
<header>
 <div class="wrap top"><h1>Teclado ESP32</h1><span id="netPill" class="pill">...</span></div>
 <nav class="wrap">
  <button type="button" data-tab="users" class="active">Usuários</button>
  <button type="button" data-tab="network">Rede</button>
  <button type="button" data-tab="system">Sistema</button>
 </nav>
</header>

<main class="wrap">

 <section id="users">
  <div class="grid two">
   <div class="card">
    <h2>Usuários <span id="userCount" class="count"></span></h2>
    <ul id="userList" class="users"><li class="empty">Carregando...</li></ul>
   </div>
   <form id="userForm" class="card" autocomplete="off">
    <h2 id="formTitle">Adicionar usuário</h2>
    <label>Nome<input id="uName" maxlength="16" required autocapitalize="words" autocorrect="off" spellcheck="false"></label>
    <p class="hint">Até 16 caracteres, sem acentos (a tela do teclado não os mostra).</p>
    <label>Senha<input id="uPin" maxlength="8" required inputmode="numeric" pattern="[0-9]{1,8}"></label>
    <p class="hint">1 a 8 dígitos, diferente da senha dos outros usuários.</p>
    <label class="check"><input id="uChallenge" type="checkbox" checked>Fazer o desafio de matemática</label>
    <p class="hint">Desmarcado: o teclado só mostra o nome, sem a conta.</p>
    <div class="row">
     <button id="uSubmit" class="primary">Adicionar</button>
     <button id="uCancel" type="button" hidden>Cancelar</button>
    </div>
   </form>
  </div>
 </section>

 <section id="network" hidden>
  <div class="grid two">
   <div class="card">
    <h2>Estado da conexão</h2>
    <dl id="netInfo" class="kv"></dl>
    <p class="hint lead" style="margin:16px 0 0">Sem acesso à rede? Abra o menu do teclado e escolha a opção 1 para ligar o hotspot de configuração.</p>
   </div>
   <form id="wifiForm" class="card" autocomplete="off">
    <h2>Configurar WiFi</h2>
    <button id="scanBtn" type="button" class="block" style="margin-bottom:14px">Procurar redes</button>
    <ul id="scanList" class="scan"></ul>
    <label>Nome da rede (SSID)<input id="wSsid" maxlength="32" required autocapitalize="none" autocorrect="off" spellcheck="false"></label>
    <label>Senha da rede<input id="wPass" type="password" maxlength="63" autocomplete="new-password"></label>
    <p class="hint">Deixe vazio em redes abertas. Depois de salvar, o teclado se conecta à nova rede e mostra o IP na tela. Para ver o IP de novo, use a opção 2 do menu do teclado.</p>
    <button class="primary block">Salvar e conectar</button>
   </form>
  </div>
 </section>

 <section id="system" hidden>
  <div class="grid two">
   <div class="col">
    <div class="card">
     <h2>Dispositivo</h2>
     <dl id="sysInfo" class="kv"></dl>
    </div>
    <div class="card">
     <h2>Reiniciar</h2>
     <p class="hint lead">Os usuários e as configurações continuam guardados.</p>
     <button id="rebootBtn" type="button" class="danger block">Reiniciar o dispositivo</button>
    </div>
   </div>
   <div class="col">
    <form id="otaForm" class="card">
     <h2>Atualizar firmware</h2>
     <p class="hint lead">Envie o arquivo <code>firmware.bin</code> gerado pelo PlatformIO ou pela Arduino IDE (Sketch &gt; Exportar binário compilado).</p>
     <label>Arquivo<input id="otaFile" type="file" accept=".bin,application/octet-stream" required></label>
     <progress id="otaBar" max="100" value="0" hidden></progress>
     <button id="otaBtn" class="primary block">Enviar e instalar</button>
    </form>
   </div>
  </div>
 </section>
</main>

<div id="toast" role="status" aria-live="polite"></div>

<script>
'use strict';
const $ = s => document.querySelector(s);
const $$ = s => Array.from(document.querySelectorAll(s));
const TABS = ['users', 'network', 'system'];
const sleep = ms => new Promise(r => setTimeout(r, ms));
let users = [], maxUsers = 0, editing = -1, ssidLoaded = false;

function esc(s) {
  return String(s).replace(/[&<>"']/g, c => ({'&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;', "'": '&#39;'}[c]));
}
function dots(n) { return '\u2022'.repeat(n); }
// Phone keyboards autocorrect to accented words and smart quotes (Joao -> Jo\u00e3o),
// but the device only accepts ASCII names.
function asciiName(s) {
  return s.normalize('NFKD').replace(/[\u0300-\u036f]/g, '')
    .replace(/[\u2018\u2019\u201a\u201b\u2032]/g, "'").replace(/[\u201c\u201d\u201e\u2033]/g, '"')
    .replace(/[\u2010-\u2015]/g, '-').replace(/\s+/g, ' ').trim();
}

function toast(msg, bad) {
  const t = $('#toast');
  t.textContent = msg;
  t.className = bad ? 'show bad' : 'show';
  clearTimeout(toast.timer);
  toast.timer = setTimeout(() => { t.className = ''; }, 4500);
}

async function api(path, data) {
  const opts = data ? {method: 'POST', body: new URLSearchParams(data)} : {cache: 'no-store'};
  const res = await fetch(path, opts);
  let body = {};
  try { body = await res.json(); } catch (e) { /* empty or non-JSON body */ }
  if (!res.ok) throw new Error(body.error || ('Erro ' + res.status));
  return body;
}

/* ---------- Tabs ---------- */
function showTab(id) {
  if (TABS.indexOf(id) < 0) id = 'users';
  $$('nav button').forEach(b => b.classList.toggle('active', b.dataset.tab === id));
  TABS.forEach(t => { $('#' + t).hidden = t !== id; });
  if (location.hash !== '#' + id) history.replaceState(null, '', '#' + id);
  window.scrollTo(0, 0);
}
$$('nav button').forEach(b => { b.onclick = () => showTab(b.dataset.tab); });
window.addEventListener('hashchange', () => showTab(location.hash.slice(1)));

/* ---------- Users ---------- */
async function loadUsers() {
  try {
    const j = await api('/api/users');
    users = j.users;
    maxUsers = j.max;
    renderUsers();
  } catch (e) { toast(e.message, true); }
}

function renderUsers() {
  $('#userCount').textContent = users.length + ' / ' + maxUsers;
  const ul = $('#userList');
  if (!users.length) {
    ul.innerHTML = '<li class="empty">Nenhum usuário cadastrado.</li>';
    return;
  }
  ul.innerHTML = users.map((u, i) =>
    '<li class="user' + (i === editing ? ' editing' : '') + '" data-i="' + i + '">' +
    '<div class="who"><strong>' + esc(u.name) + '</strong><span class="pin">' + dots(u.pin.length) + '</span>' +
    '<span class="tag' + (u.challenge ? ' on' : '') + '">' + (u.challenge ? 'Com desafio' : 'Sem desafio') + '</span></div>' +
    '<div class="actions">' +
    '<button type="button" data-act="show">Mostrar</button>' +
    '<button type="button" data-act="edit">Editar</button>' +
    '<button type="button" class="danger" data-act="del">Remover</button>' +
    '</div></li>').join('');
}

$('#userList').addEventListener('click', async e => {
  const btn = e.target.closest('button');
  if (!btn) return;
  const li = btn.closest('li');
  const i = Number(li.dataset.i);
  const u = users[i];
  if (!u) return;

  if (btn.dataset.act === 'show') {
    const show = btn.textContent === 'Mostrar';
    li.querySelector('.pin').textContent = show ? u.pin : dots(u.pin.length);
    btn.textContent = show ? 'Ocultar' : 'Mostrar';
  } else if (btn.dataset.act === 'edit') {
    startEdit(i);
  } else if (btn.dataset.act === 'del') {
    if (!confirm('Remover ' + u.name + '?')) return;
    btn.disabled = true;
    try {
      await api('/api/users/delete', {index: i});
      toast(u.name + ' foi removido.');
      if (editing === i) resetForm();
      else if (editing > i) editing--;
      await loadUsers();
    } catch (err) {
      toast(err.message, true);
      btn.disabled = false;
    }
  }
});

function startEdit(i) {
  editing = i;
  $('#uName').value = users[i].name;
  $('#uPin').value = users[i].pin;
  $('#uChallenge').checked = users[i].challenge;
  $('#formTitle').textContent = 'Editar usuário';
  $('#uSubmit').textContent = 'Salvar alterações';
  $('#uCancel').hidden = false;
  renderUsers();
  $('#userForm').scrollIntoView({behavior: 'smooth', block: 'nearest'});
  $('#uName').focus();
}

function resetForm() {
  editing = -1;
  $('#userForm').reset();
  $('#formTitle').textContent = 'Adicionar usuário';
  $('#uSubmit').textContent = 'Adicionar';
  $('#uCancel').hidden = true;
  renderUsers();
}
$('#uCancel').onclick = resetForm;

$('#userForm').addEventListener('submit', async e => {
  e.preventDefault();
  $('#uName').value = asciiName($('#uName').value);
  const data = {name: $('#uName').value, pin: $('#uPin').value.trim(), challenge: $('#uChallenge').checked ? 1 : 0};
  const btn = $('#uSubmit');
  btn.disabled = true;
  try {
    if (editing >= 0) {
      await api('/api/users/update', Object.assign({index: editing}, data));
      toast('Usuário atualizado.');
    } else {
      await api('/api/users', data);
      toast('Usuário adicionado.');
    }
    resetForm();
    await loadUsers();
  } catch (err) { toast(err.message, true); }
  btn.disabled = false;
});

/* ---------- Status ---------- */
function fmtUptime(s) {
  const d = Math.floor(s / 86400), h = Math.floor(s % 86400 / 3600), m = Math.floor(s % 3600 / 60);
  return (d ? d + ' d ' : '') + (d || h ? h + ' h ' : '') + m + ' min';
}
function signal(rssi) { return Math.max(0, Math.min(100, 2 * (rssi + 100))) + '%'; }
function kv(rows) { return rows.map(r => '<dt>' + esc(r[0]) + '</dt><dd>' + esc(r[1]) + '</dd>').join(''); }

async function loadStatus() {
  const pill = $('#netPill');
  let s;
  try { s = await api('/api/status'); } catch (e) {
    pill.className = 'pill';
    pill.textContent = 'Sem resposta';
    return;
  }
  const w = s.wifi;
  pill.className = 'pill' + (w.connected ? ' on' : '');
  pill.textContent = w.connected ? w.ssid : (w.ap ? 'Modo configuração' : 'Desconectado');

  const net = [['Estado', w.connected ? 'Conectado' : (w.configured ? 'Conectando...' : 'Nenhuma rede configurada')]];
  if (w.configured) net.push(['Rede', w.ssid]);
  if (w.connected) {
    net.push(['Endereço IP', w.ip], ['Sinal', signal(w.rssi) + ' (' + w.rssi + ' dBm)'], ['Endereço local', 'http://' + s.hostname + '.local/']);
  }
  if (w.ap) net.push(['Hotspot de configuração', w.apSsid], ['IP do hotspot', w.apIp]);
  $('#netInfo').innerHTML = kv(net);

  $('#sysInfo').innerHTML = kv([
    ['Firmware', 'v' + s.version],
    ['Nome na rede', s.hostname + '.local'],
    ['Ligado há', fmtUptime(s.uptime)],
    ['Memória livre', Math.round(s.heap / 1024) + ' KB'],
    ['Usuários', s.users.count + ' / ' + s.users.max]
  ]);
  if (!ssidLoaded) { $('#wSsid').value = w.ssid; ssidLoaded = true; }
}

/* ---------- WiFi ---------- */
$('#scanBtn').onclick = async () => {
  const btn = $('#scanBtn');
  btn.disabled = true;
  btn.textContent = 'Procurando...';
  try {
    await api('/api/wifi/scan', {});
    let j = {scanning: true};
    for (let k = 0; k < 20 && j.scanning; k++) {
      await sleep(1000);
      j = await api('/api/wifi/scan');
    }
    renderScan(j.networks || []);
  } catch (err) { toast(err.message, true); }
  btn.disabled = false;
  btn.textContent = 'Procurar redes';
};

function renderScan(list) {
  const best = new Map();
  list.forEach(n => { if (!best.has(n.ssid) || best.get(n.ssid).rssi < n.rssi) best.set(n.ssid, n); });
  const nets = Array.from(best.values()).sort((a, b) => b.rssi - a.rssi);
  $('#scanList').innerHTML = nets.length ? nets.map(n =>
    '<li><button type="button" data-ssid="' + esc(n.ssid) + '"><span>' + esc(n.ssid) +
    (n.secure ? '' : ' (aberta)') + '</span><span class="count">' + signal(n.rssi) + '</span></button></li>').join('')
    : '<li class="empty">Nenhuma rede encontrada.</li>';
}

$('#scanList').addEventListener('click', e => {
  const b = e.target.closest('button');
  if (!b) return;
  $('#wSsid').value = b.dataset.ssid;
  $('#wPass').value = '';
  $('#wPass').focus();
});

$('#wifiForm').addEventListener('submit', async e => {
  e.preventDefault();
  const ssid = $('#wSsid').value, pass = $('#wPass').value;
  if (!confirm('Conectar à rede "' + ssid + '"? A conexão atual será interrompida.')) return;
  try {
    await api('/api/wifi', {ssid: ssid, pass: pass});
    $('#wPass').value = '';
    toast('Salvo. Conectando a ' + ssid + '... O novo IP aparece na tela do teclado.');
  } catch (err) { toast(err.message, true); }
});

/* ---------- System ---------- */
$('#otaForm').addEventListener('submit', e => {
  e.preventDefault();
  const file = $('#otaFile').files[0];
  if (!file) return;
  if (!confirm('Instalar ' + file.name + '? O dispositivo vai reiniciar.')) return;

  const bar = $('#otaBar'), btn = $('#otaBtn');
  const finish = (ok, msg) => {
    btn.disabled = false;
    btn.textContent = 'Enviar e instalar';
    if (!ok) bar.hidden = true;
    toast(msg, !ok);
  };
  const fd = new FormData();
  fd.append('firmware', file, file.name);
  const xhr = new XMLHttpRequest();
  xhr.upload.onprogress = ev => { if (ev.lengthComputable) bar.value = Math.round(ev.loaded * 100 / ev.total); };
  xhr.onload = () => {
    let j = {};
    try { j = JSON.parse(xhr.responseText); } catch (x) { /* ignore */ }
    if (xhr.status === 200) {
      finish(true, 'Firmware instalado. Reiniciando...');
      setTimeout(() => location.reload(), 10000);
    } else {
      finish(false, j.error || ('Erro ' + xhr.status));
    }
  };
  xhr.onerror = () => finish(false, 'Falha no envio do arquivo.');
  bar.value = 0;
  bar.hidden = false;
  btn.disabled = true;
  btn.textContent = 'Enviando...';
  xhr.open('POST', '/api/update');
  xhr.send(fd);
});

$('#rebootBtn').onclick = async () => {
  if (!confirm('Reiniciar o dispositivo agora?')) return;
  try {
    await api('/api/reboot', {});
    toast('Reiniciando...');
    setTimeout(() => location.reload(), 8000);
  } catch (err) { toast(err.message, true); }
};

showTab(location.hash.slice(1));
loadUsers();
loadStatus();
setInterval(loadStatus, 10000);
</script>
</body>
</html>
)rawliteral";
