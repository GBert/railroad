#ifndef __CONFIG_H__
#define __CONFIG_H__

#ifdef ARDUINO
#include <Arduino.h>
#else
#include <cstdint>
#endif

#include "RR32Can/Constants.h"

#define STD_OFF (0U)
#define STD_ON (1U)

/*
 * Logging Features
 */
#define LOG_S88_BITS STD_OFF
#define LOG_BUTTON_PRESS STD_OFF
#define LOG_ACTIONLIST STD_OFF
#define LOG_BUTTON_MAPPING STD_OFF

#define LOG_CAN_IN_MSG STD_OFF
#define LOG_CAN_RAW_MSG_IN STD_OFF
#define LOG_CAN_OUT_MSG STD_OFF
#define LOG_PING STD_OFF

// Levels: 0 - off. 1 - log received elements. 2 - log received events. 10 - log
// every processing step.
#define LOG_CONFIG_DATA_STREAM_LEVEL_OFF (0)
#define LOG_CONFIG_DATA_STREAM_LEVEL_ELEMENTS (1)
#define LOG_CONFIG_DATA_STREAM_LEVEL_EVENTS (2)
#define LOG_CONFIG_DATA_STREAM_LEVEL_PACKETS (3)
#define LOG_CONFIG_DATA_STREAM_LEVEL_ALL (10)

#define LOG_CONFIG_DATA_STREAM_LEVEL (LOG_CONFIG_DATA_STREAM_LEVEL_ELEMENTS)
#define LOG_DUMP_CONFIG_DATA_STREAM STD_OFF

constexpr uint32_t RR32CanUUID = (RR32Can::enthusiastStationBaseUUID + 11);

#endif  // __CONFIG_H__