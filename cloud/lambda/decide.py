"""Cloud decision for the factory hub.

Triggered by the IoT rule on every telemetry message. When any online sensor node is too hot
or detects gas, it asks the hub (through its Device Shadow) to run the ventilation fan (relay)
and open the vent valve (servo); it switches both off again once readings fall below the
lower thresholds (hysteresis, so actuators do not flap around a single limit).
"""
import json
import os

import boto3

iot = boto3.client("iot-data")

TEMP_ON = float(os.environ["TEMP_ON"])
TEMP_OFF = float(os.environ["TEMP_OFF"])
GAS_ON = float(os.environ["GAS_ON"])
GAS_OFF = float(os.environ["GAS_OFF"])


def desired_state(thing):
    try:
        shadow = json.loads(iot.get_thing_shadow(thingName=thing)["payload"].read())
    except iot.exceptions.ResourceNotFoundException:
        return {}
    return shadow.get("state", {}).get("desired", {}) or {}


def hazard(temp, gas, active):
    if active:
        return temp > TEMP_OFF or gas > GAS_OFF
    return temp >= TEMP_ON or gas >= GAS_ON


def handler(event, context):
    thing = event["device"]
    nodes = [n for n in event.get("nodes", []) if n.get("online")]
    if not nodes:
        return {"skipped": "no online nodes"}

    temp = max(n["temp"] for n in nodes)
    gas = max(n["gas"] for n in nodes)
    desired = desired_state(thing)
    active = desired.get("fan") == "on"
    danger = hazard(temp, gas, active)

    if danger == active and "fan" in desired:
        return {"changed": False, "temp": temp, "gas": gas}

    state = {"fan": "on" if danger else "off", "valve": "open" if danger else "closed"}
    iot.update_thing_shadow(thingName=thing, payload=json.dumps({"state": {"desired": state}}))
    decision = {"changed": True, "thing": thing, "temp": temp, "gas": gas, "desired": state}
    print(json.dumps(decision))  # lands in CloudWatch Logs
    return decision
