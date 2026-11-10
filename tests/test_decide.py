"""Cloud decision Lambda (cloud/lambda/decide.py): hysteresis and shadow updates, with a fake AWS client."""
import importlib
import io
import json
import os
import sys
import types
import unittest
from pathlib import Path

LAMBDA_DIR = Path(__file__).resolve().parents[1] / "cloud" / "lambda"
CONFIG = json.loads((LAMBDA_DIR.parent / "config.json").read_text(encoding="utf-8"))["thresholds"]


class NotFound(Exception):
    pass


class FakeIotData:
    exceptions = types.SimpleNamespace(ResourceNotFoundException=NotFound)

    def __init__(self):
        self.shadow = None
        self.updates = []

    def get_thing_shadow(self, thingName):
        if self.shadow is None:
            raise NotFound()
        return {"payload": io.BytesIO(json.dumps(self.shadow).encode())}

    def update_thing_shadow(self, thingName, payload):
        desired = json.loads(payload)["state"]["desired"]
        self.updates.append(desired)
        self.shadow = {"state": {"desired": desired}}


def event(temp, gas, online=True):
    return {"device": "hub01", "nodes": [{"id": 0, "online": online, "temp": temp, "hum": 40, "gas": gas, "motion": False}]}


class DecideTest(unittest.TestCase):
    def setUp(self):
        os.environ.update(TEMP_ON=str(CONFIG["temp_on"]), TEMP_OFF=str(CONFIG["temp_off"]),
                          GAS_ON=str(CONFIG["gas_on"]), GAS_OFF=str(CONFIG["gas_off"]))
        self.iot = FakeIotData()
        sys.modules["boto3"] = types.SimpleNamespace(client=lambda name: self.iot)
        sys.path.insert(0, str(LAMBDA_DIR))
        sys.modules.pop("decide", None)
        self.decide = importlib.import_module("decide")

    def tearDown(self):
        sys.path.remove(str(LAMBDA_DIR))
        sys.modules.pop("boto3", None)
        sys.modules.pop("decide", None)

    def run_steps(self, steps):
        return [self.decide.handler(event(*s), None) for s in steps]

    def test_temperature_hysteresis(self):
        on, off = CONFIG["temp_on"], CONFIG["temp_off"]
        self.run_steps([(on - 10, 0), (on + 1, 0), (on - 1, 0), (off - 1, 0)])
        self.assertEqual([u["fan"] for u in self.iot.updates], ["off", "on", "off"])

    def test_gas_hysteresis(self):
        on, off = CONFIG["gas_on"], CONFIG["gas_off"]
        self.run_steps([(20, off - 10), (20, on + 5), (20, off + 5), (20, off - 1)])
        self.assertEqual([u["valve"] for u in self.iot.updates], ["closed", "open", "closed"])

    def test_no_update_when_nothing_changes(self):
        self.run_steps([(20, 10), (20, 10), (21, 11)])
        self.assertEqual(len(self.iot.updates), 1)

    def test_offline_nodes_are_ignored(self):
        result = self.decide.handler(event(90, 250, online=False), None)
        self.assertIn("skipped", result)
        self.assertEqual(self.iot.updates, [])


if __name__ == "__main__":
    unittest.main()
