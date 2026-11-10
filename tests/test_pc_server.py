"""Smoke test of pc-dashboard/server.py: starts it on a free port with a temporary config and database."""
import base64
import json
import shutil
import socket
import subprocess
import sys
import tempfile
import time
import unittest
import urllib.error
import urllib.request
from pathlib import Path

DASH = Path(__file__).resolve().parents[1] / "pc-dashboard"
KEY = "test-key"
USER, PASSWORD = "admin", "test-password"
AUTH = "Basic " + base64.b64encode(f"{USER}:{PASSWORD}".encode()).decode()
SNAPSHOT = {
    "state": "armed", "gasAlarm": False, "relay": True, "door": False, "armLeft": 0, "armDelay": 10,
    "gasLimit": 100, "tempHigh": 30, "uptime": 125,
    "nodes": [{"id": 0, "online": True, "temp": 24, "hum": 45, "gas": 37, "motion": True, "age": 1}],
    "log": [{"t": "18:00:49", "m": "Kapı panelden açıldı", "u": True}, {"t": "18:00:40", "m": "Düğüm 0 bağlandı", "u": False}],
}


def free_port():
    with socket.socket() as s:
        s.bind(("127.0.0.1", 0))
        return s.getsockname()[1]


class PcServerTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.tmp = Path(tempfile.mkdtemp())
        for name in ("server.py", "index.html"):
            shutil.copy(DASH / name, cls.tmp / name)
        cls.port = free_port()
        config = json.loads((DASH / "config.example.json").read_text(encoding="utf-8"))
        config.update(host="127.0.0.1", port=cls.port, api_key=KEY, database="test.db",
                      dashboard_user=USER, dashboard_password=PASSWORD, command_timeout_s=2)
        (cls.tmp / "config.json").write_text(json.dumps(config), encoding="utf-8")
        cls.proc = subprocess.Popen([sys.executable, "server.py"], cwd=cls.tmp, stdout=subprocess.DEVNULL, stderr=subprocess.PIPE)
        cls.base = f"http://127.0.0.1:{cls.port}"
        for _ in range(50):
            try:
                urllib.request.urlopen(cls.base + "/", timeout=0.5)
            except urllib.error.HTTPError as e:
                e.close()
                break  # 401 means the server is up
            except OSError:
                time.sleep(0.1)

    @classmethod
    def tearDownClass(cls):
        cls.proc.terminate()
        cls.proc.wait(5)
        cls.proc.stderr.close()
        shutil.rmtree(cls.tmp, ignore_errors=True)

    def request(self, path, body=None, headers=None, method=None):
        req = urllib.request.Request(self.base + path, data=body, method=method, headers=headers or {})
        try:
            with urllib.request.urlopen(req, timeout=3) as r:
                return r.status, json.loads(r.read() or b"null")
        except urllib.error.HTTPError as e:
            e.close()
            return e.code, None

    def telemetry(self, snapshot, key=KEY):
        body = snapshot if isinstance(snapshot, bytes) else json.dumps(snapshot, ensure_ascii=False).encode("utf-8")
        return self.request("/api/telemetry", body, {"Content-Type": "application/json", "X-Api-Key": key}, "POST")

    def get(self, path, auth=AUTH):
        return self.request(path, headers={"Authorization": auth} if auth else {})

    def command(self, cmd):
        return self.request("/api/command", json.dumps({"cmd": cmd}).encode(),
                            {"Content-Type": "application/json", "Authorization": AUTH}, "POST")

    def test_telemetry_flow(self):
        self.assertEqual(self.telemetry(SNAPSHOT)[0], 200)
        self.assertEqual(self.telemetry(SNAPSHOT)[0], 200)  # same events again must not duplicate
        status, latest = self.get("/api/latest")
        self.assertEqual(status, 200)
        self.assertTrue(latest["online"])
        self.assertEqual(latest["snapshot"]["state"], "armed")
        self.assertGreaterEqual(len(self.get("/api/history?minutes=5")[1]), 2)
        self.assertEqual(len([e for e in self.get("/api/events")[1] if e["device_time"] in ("18:00:49", "18:00:40")]), 2)

    def test_rejects_wrong_key_and_bad_body(self):
        self.assertEqual(self.telemetry({}, key="wrong")[0], 401)
        self.assertEqual(self.telemetry(b'{"m":"\xfd"}')[0], 400)
        self.assertEqual(self.telemetry(b"not json")[0], 400)

    def test_dashboard_requires_login(self):
        for path in ("/", "/api/latest", "/api/commands"):
            self.assertEqual(self.get(path, auth=None)[0], 401, path)
            self.assertEqual(self.get(path, auth="Basic d3Jvbmc6d3Jvbmc=")[0], 401, path)

    def test_command_round_trip(self):
        status, queued = self.command("relay_on")
        self.assertEqual(status, 202)
        _, reply = self.telemetry(SNAPSHOT)
        self.assertEqual(reply["commands"], [{"id": queued["id"], "cmd": "relay_on"}])
        self.assertEqual(self.telemetry(SNAPSHOT)[1]["commands"], [])  # delivered only once
        self.telemetry(dict(SNAPSHOT, acks=[{"id": queued["id"], "ok": True}]))
        rows = {c["id"]: c["status"] for c in self.get("/api/commands")[1]}
        self.assertEqual(rows[queued["id"]], "done")

    def test_unknown_command_is_rejected(self):
        self.assertEqual(self.command("format_disk")[0], 400)
        self.assertEqual(self.request("/api/command", b"[]", {"Authorization": AUTH}, "POST")[0], 400)

    def test_unacknowledged_command_expires(self):
        _, queued = self.command("door")
        time.sleep(2.2)  # command_timeout_s = 2 in this test
        rows = {c["id"]: c["status"] for c in self.get("/api/commands")[1]}
        self.assertEqual(rows[queued["id"]], "expired")
        self.assertNotIn({"id": queued["id"], "cmd": "door"}, self.telemetry(SNAPSHOT)[1]["commands"])


if __name__ == "__main__":
    unittest.main()
