#pragma once
// Logging shim for web simulator.
// The real Logging.h depends on ESP32's HWCDC type and redefines Serial.
// In the simulator, we provide no-op LOG macros and leave Serial alone.

#include <HardwareSerial.h>

#ifndef LOG_LEVEL
#define LOG_LEVEL 0
#endif

// The real Logging.h defines logSerial as a reference to the underlying Serial
// (HWCDC& on ESP32). In the simulator, it's just a reference to our HardwareSerial.
// Firmware code uses logSerial for raw serial access without deprecation warnings.
static HardwareSerial& logSerial = Serial;

// LOG_* macros are no-ops in the simulator (ENABLE_SERIAL_LOG is not defined)
#define LOG_DBG(origin, format, ...)
#define LOG_ERR(origin, format, ...)
#define LOG_INF(origin, format, ...)

// logPrintf stub
inline void logPrintf(const char*, const char*, const char*, ...) {}
