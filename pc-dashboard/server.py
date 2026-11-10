"""PC dashboard for the IoT security hub.

The ESP32 hub POSTs its status JSON every few seconds to /api/telemetry. This server keeps the
latest snapshot, stores sensor samples and events in SQLite, serves the dashboard page and queues
remote commands. The hub has no inbound connection from the internet: it picks pending commands
up from the telemetry reply and acknowledges them in its next upload.
Standard library only: run with `python server.py` (start.ps1 also opens a Cloudflare tunnel).
"""
import base64
import hmac
import json
import sqlite3
import threading
import time
from datetime import datetime
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
from urllib.parse import parse_qs, urlparse

BASE_DIR = Path(__file__).resolve().parent
CONFIG = json.loads((BASE_DIR / "config.json").read_text(encoding="utf-8"))
DB_PATH = BASE_DIR / CONFIG["database"]
PAGE_PATH = BASE_DIR / "index.html"
MAX_BODY_BYTES = 64 * 1024
ALLOWED_COMMANDS = set(CONFIG["allowed_commands"])
_EXPECTED_AUTH = "Basic " + base64.b64encode(
    f"{CONFIG['dashboard_user']}:{CONFIG['dashboard_password']}".encode("utf-8")).decode("ascii")

_db_lock = threading.Lock()
_latest = {"snapshot": None, "received_at": 0.0}


def connect():
    conn = sqlite3.connect(DB_PATH)
    conn.row_factory = sqlite3.Row
    return conn


def init_db():
    with connect() as conn:
        conn.executescript(
            """
            CREATE TABLE IF NOT EXISTS samples (
                ts REAL NOT NULL, node INTEGER NOT NULL, temp INTEGER, hum INTEGER,
                gas INTEGER, motion INTEGER, state TEXT
            );
            CREATE INDEX IF NOT EXISTS idx_samples_ts ON samples (ts);
            CREATE TABLE IF NOT EXISTS events (
                id INTEGER PRIMARY KEY AUTOINCREMENT, ts REAL NOT NULL, day TEXT NOT NULL,
                device_time TEXT NOT NULL, message TEXT NOT NULL, urgent INTEGER NOT NULL,
                UNIQUE (day, device_time, message)
            );
            CREATE TABLE IF NOT EXISTS commands (
                id INTEGER PRIMARY KEY AUTOINCREMENT, created REAL NOT NULL, updated REAL NOT NULL,
                command TEXT NOT NULL, status TEXT NOT NULL
            );
            """
        )


def expire_commands(conn, now):
    # A command the hub has not acknowledged in time must not fire later by surprise.
    conn.execute("UPDATE commands SET status = 'expired', updated = ? WHERE status IN ('pending', 'sent') AND created < ?",
                 (now, now - CONFIG["command_timeout_s"]))


def store(snapshot):
    """Saves one upload and returns the commands the hub should run now."""
    now = time.time()
    day = datetime.now().strftime("%Y-%m-%d")
    with _db_lock, connect() as conn:
        for node in snapshot.get("nodes", []):
            if node.get("online"):
                conn.execute(
                    "INSERT INTO samples VALUES (?, ?, ?, ?, ?, ?, ?)",
                    (now, node["id"], node["temp"], node["hum"], node["gas"], int(node["motion"]),
                     snapshot.get("state")),
                )
        # The hub resends its last log entries every time; the unique key drops duplicates.
        for entry in reversed(snapshot.get("log", [])):
            conn.execute(
                "INSERT OR IGNORE INTO events (ts, day, device_time, message, urgent) VALUES (?, ?, ?, ?, ?)",
                (now, day, entry["t"], entry["m"], int(entry.get("u", False))),
            )
        for ack in snapshot.get("acks", []):
            conn.execute("UPDATE commands SET status = ?, updated = ? WHERE id = ? AND status = 'sent'",
                         ("done" if ack.get("ok") else "failed", now, int(ack["id"])))
        expire_commands(conn, now)
        pending = conn.execute("SELECT id, command FROM commands WHERE status = 'pending' ORDER BY id").fetchall()
        conn.executemany("UPDATE commands SET status = 'sent', updated = ? WHERE id = ?", [(now, r["id"]) for r in pending])
    _latest["snapshot"] = snapshot
    _latest["received_at"] = now
    return [{"id": r["id"], "cmd": r["command"]} for r in pending]


def queue_command(command):
    now = time.time()
    with _db_lock, connect() as conn:
        cur = conn.execute("INSERT INTO commands (created, updated, command, status) VALUES (?, ?, ?, 'pending')",
                           (now, now, command))
        return cur.lastrowid


def commands(limit):
    now = time.time()
    with _db_lock, connect() as conn:
        expire_commands(conn, now)
        rows = conn.execute("SELECT id, created, updated, command, status FROM commands ORDER BY id DESC LIMIT ?",
                            (limit,)).fetchall()
    return [dict(r) for r in rows]


def latest():
    age = time.time() - _latest["received_at"]
    return {
        "snapshot": _latest["snapshot"],
        "age": round(age, 1) if _latest["snapshot"] else None,
        "online": _latest["snapshot"] is not None and age < CONFIG["device_timeout_s"],
    }


def history(minutes):
    since = time.time() - minutes * 60
    with connect() as conn:
        rows = conn.execute(
            "SELECT ts, node, temp, hum, gas, motion FROM samples WHERE ts >= ? ORDER BY ts", (since,)
        ).fetchall()
    return [dict(r) for r in rows]


def events(limit):
    with connect() as conn:
        rows = conn.execute(
            "SELECT ts, device_time, message, urgent FROM events ORDER BY id DESC LIMIT ?", (limit,)
        ).fetchall()
    return [dict(r) for r in rows]


class Handler(BaseHTTPRequestHandler):
    def log_message(self, fmt, *args):  # keep the console quiet; telemetry arrives every few seconds
        pass

    def send_json(self, payload, status=200):
        body = json.dumps(payload, ensure_ascii=False).encode("utf-8")
        self.send_response(status)
        self.send_header("Content-Type", "application/json; charset=utf-8")
        self.send_header("Content-Length", str(len(body)))
        self.send_header("Cache-Control", "no-store")
        self.end_headers()
        self.wfile.write(body)

    # Every dashboard route needs the login; the page is reachable from the internet through the tunnel.
    def authorized(self):
        if hmac.compare_digest(self.headers.get("Authorization", ""), _EXPECTED_AUTH):
            return True
        self.send_response(401)
        self.send_header("WWW-Authenticate", 'Basic realm="IoT dashboard", charset="UTF-8"')
        self.send_header("Content-Length", "0")
        self.end_headers()
        return False

    def read_json(self):
        length = int(self.headers.get("Content-Length", 0))
        if length <= 0 or length > MAX_BODY_BYTES:
            return None
        try:
            return json.loads(self.rfile.read(length))
        except (json.JSONDecodeError, UnicodeDecodeError):
            return None

    def do_GET(self):
        if not self.authorized():
            return
        url = urlparse(self.path)
        query = parse_qs(url.query)
        if url.path == "/":
            body = PAGE_PATH.read_bytes()
            self.send_response(200)
            self.send_header("Content-Type", "text/html; charset=utf-8")
            self.send_header("Content-Length", str(len(body)))
            self.end_headers()
            self.wfile.write(body)
        elif url.path == "/api/latest":
            self.send_json(latest())
        elif url.path == "/api/history":
            minutes = min(int(query.get("minutes", [CONFIG["history_minutes"]])[0]), 24 * 60)
            self.send_json(history(minutes))
        elif url.path == "/api/events":
            self.send_json(events(CONFIG["events_limit"]))
        elif url.path == "/api/commands":
            self.send_json(commands(CONFIG["events_limit"]))
        else:
            self.send_json({"error": "not found"}, 404)

    def do_POST(self):
        path = urlparse(self.path).path
        if path == "/api/telemetry":
            return self.handle_telemetry()
        if path == "/api/command":
            if not self.authorized():
                return
            body = self.read_json()
            command = body.get("cmd") if isinstance(body, dict) else None
            if command not in ALLOWED_COMMANDS:
                return self.send_json({"error": "unknown command"}, 400)
            return self.send_json({"id": queue_command(command), "status": "pending"}, 202)
        self.send_json({"error": "not found"}, 404)

    def handle_telemetry(self):
        if not hmac.compare_digest(self.headers.get("X-Api-Key", ""), CONFIG["api_key"]):
            return self.send_json({"error": "invalid api key"}, 401)
        snapshot = self.read_json()
        if not isinstance(snapshot, dict):
            return self.send_json({"error": "invalid json"}, 400)
        self.send_json({"commands": store(snapshot)})


def main():
    init_db()
    server = ThreadingHTTPServer((CONFIG["host"], CONFIG["port"]), Handler)
    print(f"Dashboard: http://localhost:{CONFIG['port']}  (hub posts to /api/telemetry)")
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        pass


if __name__ == "__main__":
    main()
