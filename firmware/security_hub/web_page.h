#pragma once
#include <Arduino.h>

// Dashboard served at "/". Polls /api/status and posts commands to /api/*.
const char kWebPage[] PROGMEM = R"HTML(<!doctype html>
<html lang="tr">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Güvenlik Paneli</title>
<style>
:root{--bg:#0f1419;--card:#1a222c;--text:#e6edf3;--muted:#8b98a5;--ok:#2ea043;--warn:#d29922;--bad:#f85149;--accent:#388bfd}
*{box-sizing:border-box}
body{margin:0;font-family:system-ui,Segoe UI,Roboto,sans-serif;background:var(--bg);color:var(--text);padding:16px}
main{max-width:720px;margin:0 auto;display:grid;gap:12px}
h1{font-size:20px;margin:0 0 4px}
.card{background:var(--card);border-radius:10px;padding:16px}
.state{font-size:28px;font-weight:700}
.disarmed{color:var(--muted)}.arming{color:var(--warn)}.armed{color:var(--ok)}.triggered,.gas{color:var(--bad)}
.row{display:flex;flex-wrap:wrap;gap:8px}
button{flex:1 1 140px;padding:12px;border:0;border-radius:8px;font-size:15px;font-weight:600;color:#fff;background:var(--accent);cursor:pointer}
button.ok{background:var(--ok)}button.bad{background:var(--bad)}button.muted{background:#30363d}
.grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(120px,1fr));gap:8px}
.metric{background:#11181f;border-radius:8px;padding:10px}
.metric b{display:block;font-size:22px}
.metric span{color:var(--muted);font-size:13px}
ul{list-style:none;margin:0;padding:0;max-height:320px;overflow:auto}
li{padding:6px 0;border-bottom:1px solid #30363d;font-size:14px}
li time{color:var(--muted);margin-right:8px;font-variant-numeric:tabular-nums}
.offline{opacity:.5}
li.urgent{color:var(--bad);font-weight:600}
</style>
</head>
<body>
<main>
<section class="card">
<h1>Akıllı Güvenlik Sistemi</h1>
<div id="state" class="state">Bağlanıyor...</div>
<div id="gas" class="gas"></div>
</section>
<section class="card row">
<button class="ok" onclick="cmd('arm')">Sistemi Kur</button>
<button class="bad" onclick="cmd('disarm')">Devre Dışı Bırak</button>
<button onclick="cmd('door')">Kapıyı Aç</button>
<button class="muted" onclick="cmd('test')">Modül Testi</button>
<button class="muted" onclick="cmd('setup')">Kurulum Modu</button>
<button id="relay" class="muted" onclick="toggleRelay()">Röle</button>
</section>
<section class="card"><div id="nodes" class="grid"></div></section>
<section class="card"><h1>Olay Kaydı</h1><ul id="log"></ul></section>
</main>
<script>
const STATES={disarmed:'Devre Dışı',arming:'Kuruluyor...',armed:'Kurulu',triggered:'ALARM!'};
let relayOn=false;
function esc(s){return String(s).replace(/[&<>"]/g,c=>({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;'}[c]))}
function render(s){
  const st=document.getElementById('state');
  st.textContent=STATES[s.state]||s.state; st.className='state '+s.state;
  document.getElementById('gas').textContent=s.gasAlarm?'GAZ / DUMAN ALARMI':'';
  relayOn=s.relay;
  const r=document.getElementById('relay');
  r.textContent='Röle: '+(s.relay?'Açık':'Kapalı'); r.className=s.relay?'ok':'muted';
  document.getElementById('nodes').innerHTML=s.nodes.length?s.nodes.map(n=>
    `<div class="metric ${n.online?'':'offline'}"><span>Düğüm ${n.id} ${n.online?'':'(çevrimdışı)'}</span><b>${n.temp}°C</b><span>Nem %${n.hum} · Gaz ${n.gas} · ${n.motion?'Hareket VAR':'Hareket yok'} · ${n.age} sn önce</span></div>`
  ).join(''):'<span>Henüz düğüm verisi yok</span>';
  document.getElementById('log').innerHTML=s.log.map(e=>`<li class="${e.u?'urgent':''}"><time>${esc(e.t)}</time>${esc(e.m)}</li>`).join('');
}
async function call(path,opts){
  try{const r=await fetch(path,opts); if(r.ok) render(await r.json());}catch(e){document.getElementById('state').textContent='Bağlantı yok';}
}
function cmd(name,query=''){call('/api/'+name+query,{method:'POST'})}
function toggleRelay(){cmd('relay','?on='+(relayOn?0:1))}
call('/api/status'); setInterval(()=>call('/api/status'),1500);
</script>
</body>
</html>)HTML";
