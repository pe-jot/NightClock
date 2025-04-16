#pragma once
#include <time.h>

// Board: Arduino Mkr WiFi 1010
const char* hostname =				"mkrwifi1010";

#define USER_LED_PIN				6			/* Default */
#define LED_R_PIN					5
#define LED_G_PIN					4
#define LED_GND_PIN					3
#define BRIGHTNESS_STEPS			16
#define MAX_BRIGHTNESS				(BRIGHTNESS_STEPS - 1)

#define DEBUG_BAUDRATE				115200
#define RTC_CALIB_VALUE				0			/* Seems pretty stable thus no correction needed */

#define BATT_FULL_VOLTAGE			4.2
#define BATT_EMPTY_VOLTAGE			3.3
#define BATT_CAPACITY_AH			2.6
#define BATT_CHRG_CURRENT			(BATT_CAPACITY_AH / 2)
#define BATT_MAX_INPUT_CURRENT		2.0
#define BATT_R1						330000
#define BATT_R2						1000000
#define BATT_MAX_SOURCE_VOLTAGE		((3.3 * (BATT_R1 + BATT_R2)) / BATT_R2)

#define ONE_DAY						(24 * 60 * 60)

#define UserLED(_state_)			digitalWrite(USER_LED_PIN, _state_)

enum LedState {
	OFF = 0,
	ON = 1
};

typedef struct LED_t {
	uint8_t r;
	uint8_t g;
	uint8_t b;
} LED_t;

enum StateMachine {
	AFTER_STARTUP = 0,
	IDLE,
	BED_TIME,
	BED_TIME_II,
	WAKEUP_TIME,
	WAKEUP_TIME_II,
	OFF_TIME,
	LOW_BATTERY
};

typedef struct Alarm_t {
	const char* name;
	const enum StateMachine opState;
	const struct tm time;
} Alarm_t;



#define ALARM_COUNT				3
const Alarm_t alarms[ALARM_COUNT] = { // UTC
	{
		.name = "BedTime",
		.opState = BED_TIME,
		.time = { .tm_sec = 0, .tm_min = 0, .tm_hour = 18 },
		//.time = { .tm_sec = 0, .tm_min = 50, .tm_hour = 16 },
	},
	{
		.name = "WakeupTime",
		.opState = WAKEUP_TIME,
		.time = { .tm_sec = 0, .tm_min = 0, .tm_hour = 6 },
		//.time = { .tm_sec = 0, .tm_min = 55, .tm_hour = 16 },
	},
	{
		.name = "TurnOffTime",
		.opState = OFF_TIME,
		.time = { .tm_sec = 0, .tm_min = 30, .tm_hour = 6 },
		//.time = { .tm_sec = 0, .tm_min = 0, .tm_hour = 17 },
	},
};
