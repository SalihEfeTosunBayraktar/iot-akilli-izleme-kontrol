#pragma once
#include <Arduino.h>
#include <functional>

// Receives every notable event; urgent ones are also pushed to the phone.
using EventSink = std::function<void(const String& message, bool urgent)>;
