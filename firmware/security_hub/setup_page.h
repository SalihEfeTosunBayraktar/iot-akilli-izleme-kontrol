#pragma once
#include <Arduino.h>

// Setup portal served at "/setup". Lists networks from /setup/scan and posts the form to /setup/save.
// Field names must match DeviceSettings::key().
const char kSetupPage[] PROGMEM = R"HTML(<!doctype html>
<html lang="tr">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Cihaz Kurulumu</title>
<style>
:root{--bg:#0f1419;--card:#1a222c;--text:#e6edf3;--muted:#8b98a5;--ok:#2ea043;--bad:#f85149;--accent:#388bfd;--field:#11181f;--line:#30363d}
*{box-sizing:border-box}
body{margin:0;font-family:system-ui,Segoe UI,Roboto,sans-serif;background:var(--bg);color:var(--text);padding:16px}
main{max-width:560px;margin:0 auto;display:grid;gap:12px}
h1{font-size:20px;margin:0}
h2{font-size:16px;margin:0 0 8px}
p{margin:4px 0 0;color:var(--muted);font-size:14px}
.card{background:var(--card);border-radius:10px;padding:16px;display:grid;gap:10px}
label,.lbl{display:grid;gap:4px;font-size:14px;color:var(--muted)}
input,select,textarea{width:100%;padding:10px;border:1px solid var(--line);border-radius:8px;background:var(--field);color:var(--text);font-size:16px}
textarea{min-height:90px;font-family:ui-monospace,Consolas,monospace;font-size:12px}
input[type=file]{padding:6px;font-size:14px}
.row{display:flex;gap:8px}
.row select{flex:1}
button{padding:12px;border:0;border-radius:8px;font-size:15px;font-weight:600;color:#fff;background:var(--accent);cursor:pointer}
button.muted{background:var(--line)}
button:disabled{opacity:.6}
#msg{font-weight:600;min-height:1.2em}
.ok{color:var(--ok)}.bad{color:var(--bad)}
a{color:var(--accent)}
</style>
</head>
<body>
<main>
<section class="card">
<h1>Cihaz Kurulumu</h1>
<p>Boş bırakılan alanlar kayıtlı değerini korur. Kaydedince cihaz yeniden başlar.</p>
</section>
<form id="f" class="card">
<h2>Wi-Fi</h2>
<div class="lbl">Ağ listesi
<div class="row"><select id="nets"><option value="">Taranıyor...</option></select><button type="button" class="muted" id="rescan">Yenile</button></div>
</div>
<label>Ağ adı (SSID)<input name="ssid" id="ssid" autocomplete="off" autocapitalize="off"></label>
<label>Şifre<input name="pass" type="password" autocomplete="off"></label>
<h2>AWS IoT Core</h2>
<label>Uç nokta (endpoint)<input name="endpoint" placeholder="xxxx-ats.iot.eu-central-1.amazonaws.com" autocapitalize="off"></label>
<label>Thing adı<input name="thing" autocapitalize="off"></label>
<label>Konu öneki (topic prefix)<input name="prefix" autocapitalize="off"></label>
<div class="lbl">Kök CA sertifikası (AmazonRootCA1.pem)<input type="file" data-for="rootca"><textarea name="rootca" placeholder="-----BEGIN CERTIFICATE-----"></textarea></div>
<div class="lbl">Cihaz sertifikası (certificate.pem.crt)<input type="file" data-for="cert"><textarea name="cert" placeholder="-----BEGIN CERTIFICATE-----"></textarea></div>
<div class="lbl">Özel anahtar (private.pem.key)<input type="file" data-for="key"><textarea name="key" placeholder="-----BEGIN RSA PRIVATE KEY-----"></textarea></div>
<h2>PC kontrol merkezi</h2>
<label>Telemetri adresi<input name="telemetry" type="url" placeholder="http://192.168.1.10:8080/api/telemetry" autocapitalize="off"></label>
<button id="save">Kaydet ve yeniden başlat</button>
<div id="msg"></div>
<p><a href="/panel">Yerel panele git</a></p>
</form>
</main>
<script>
const $=id=>document.getElementById(id);
let tries=0;
function show(text,ok){const m=$('msg');m.textContent=text;m.className=ok?'ok':'bad'}
async function scan(){
  try{
    const s=await (await fetch('/setup/scan')).json();
    if(s.scanning&&++tries<15){setTimeout(scan,2000);return}
    tries=0;
    const nets=s.networks.sort((a,b)=>b.rssi-a.rssi);
    $('nets').innerHTML='<option value="">'+(nets.length?'Ağ seçin ('+nets.length+')':'Ağ bulunamadı')+'</option>';
    for(const n of nets){const o=document.createElement('option');o.value=n.ssid;o.textContent=n.ssid+'  '+n.rssi+' dBm'+(n.open?'  (açık)':'');$('nets').appendChild(o)}
  }catch(e){$('nets').innerHTML='<option value="">Tarama başarısız</option>'}
}
$('nets').onchange=e=>{if(e.target.value)$('ssid').value=e.target.value};
$('rescan').onclick=()=>{tries=0;$('nets').innerHTML='<option value="">Taranıyor...</option>';scan()};
for(const input of document.querySelectorAll('input[type=file]')){
  input.onchange=()=>{
    const file=input.files[0];if(!file)return;
    const r=new FileReader();
    r.onload=()=>{document.querySelector('textarea[name='+input.dataset.for+']').value=r.result};
    r.readAsText(file);
  };
}
function pemOk(v){return !v||(v.includes('-----BEGIN')&&v.includes('-----END'))}
$('f').onsubmit=async e=>{
  e.preventDefault();
  const data=new FormData($('f'));
  for(const k of ['rootca','cert','key']){if(!pemOk(data.get(k).trim())){show('PEM alanında BEGIN/END satırı yok: '+k,false);return}}
  $('save').disabled=true;show('Kaydediliyor...',true);
  try{
    const r=await fetch('/setup/save',{method:'POST',body:new URLSearchParams(data)});
    const s=await r.json();show(s.message,s.ok);
    if(!s.ok)$('save').disabled=false;
  }catch(err){show('Cihaza ulaşılamadı',false);$('save').disabled=false}
};
scan();
</script>
</body>
</html>)HTML";
