#pragma once
#include <time.h>

// Board: Arduino Nano 33 IoT
const char* hostname =				"nano33iot";

#define PWR_LED_PIN					12			/* Re-pinned */
#define USER_LED_PIN				13			/* Default */
#define LED_R_PIN					3
#define LED_G_PIN					2
#define BRIGHTNESS_STEPS			16

#define DEBUG_BAUDRATE				115200
#define RTC_CALIB_VALUE				127			/* Empirically determined in earlier tests */

#define ONE_DAY						(24 * 60 * 60)

#define PowerLED(_state_)			digitalWrite(PWR_LED_PIN, _state_)
#define UserLED(_state_)			digitalWrite(USER_LED_PIN, _state_)

enum LedState {
	OFF = 0,
	ON = 1
};

typedef struct LED_t {
	uint8_t r;
	uint8_t g;
} LED_t;

enum StateMachine {
	AFTER_STARTUP = 0,
	IDLE,
	TIME_SYNC,
	BED_TIME,
	BED_TIME_II,
	WAKEUP_TIME,
	WAKEUP_TIME_II,
	OFF_TIME
};

typedef struct Alarm_t {
	const char* name;
	const enum StateMachine opState;
	const struct tm time;
} Alarm_t;



#define PRODUCTION
#ifdef PRODUCTION
#define ALARM_COUNT				5
const Alarm_t alarms[ALARM_COUNT] = { // UTC
	{
		.name = "TimeSync",
		.opState = TIME_SYNC,
		.time = { .tm_sec = 0, .tm_min = 0, .tm_hour = 18 },
	},
	{
		.name = "BedTime",
		.opState = BED_TIME,
		.time = { .tm_sec = 0, .tm_min = 30, .tm_hour = 18 },
	},
	{
		.name = "WakeupTime",
		.opState = WAKEUP_TIME,
		.time = { .tm_sec = 0, .tm_min = 0, .tm_hour = 5 },
	},
	{
		.name = "TurnOffTime",
		.opState = OFF_TIME,
		.time = { .tm_sec = 0, .tm_min = 30, .tm_hour = 6 },
	},
	{
		.name = "TimeSync",
		.opState = TIME_SYNC,
		.time = { .tm_sec = 0, .tm_min = 0, .tm_hour = 9 },
	},
#else
#define ALARM_COUNT				3
const Alarm_t alarms[ALARM_COUNT] = { // UTC
	{
		.name = "BedTime",
		.opState = BED_TIME,
		.time = { .tm_sec = 0, .tm_min = 30, .tm_hour = 12 },
	},
	{
		.name = "WakeupTime",
		.opState = WAKEUP_TIME,
		.time = { .tm_sec = 0, .tm_min = 0, .tm_hour = 5 },
	},
	{
		.name = "TurnOffTime",
		.opState = OFF_TIME,
		.time = { .tm_sec = 0, .tm_min = 30, .tm_hour = 6 },
	},
#endif
};
