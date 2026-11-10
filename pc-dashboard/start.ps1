# Starts the dashboard server and publishes it to the internet through a Cloudflare quick tunnel.
# The public https://*.trycloudflare.com address is printed by cloudflared below; it changes on every start.
# Ctrl+C stops both.
$ErrorActionPreference = 'Stop'
$root = $PSScriptRoot
$cfg = Get-Content (Join-Path $root 'config.json') -Raw | ConvertFrom-Json

$server = Start-Process python -ArgumentList 'server.py' -WorkingDirectory $root -PassThru -WindowStyle Minimized
Write-Host "Dashboard (LAN): http://localhost:$($cfg.port)  - waiting for the public tunnel address..."
try {
    & cloudflared tunnel --no-autoupdate --url "http://localhost:$($cfg.port)"
}
finally {
    Stop-Process -Id $server.Id -Force -ErrorAction SilentlyContinue
}
