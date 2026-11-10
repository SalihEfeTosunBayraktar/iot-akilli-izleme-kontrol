#pragma once
#include <Arduino.h>
#include "AlarmSystem.h"
#include "Door.h"
#include "EventLog.h"
#include "NodeMonitor.h"
#include "Relay.h"

// Builds the status JSON shared by the local web panel and the PC telemetry upload.
class StatusReport {
 public:
  StatusReport(AlarmSystem& alarm, NodeMonitor& nodes, EventLog& log, Relay& relay, Door& door)
      : alarm_(alarm), nodes_(nodes), log_(log), relay_(relay), door_(door) {}
  String toJson() const;

 private:
  AlarmSystem& alarm_;
  NodeMonitor& nodes_;
  EventLog& log_;
  Relay& relay_;
  Door& door_;
};
