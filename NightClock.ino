#include "credentials.h"
#include "config.h"
#include <WiFiNINA.h>
#include <RTCZero.h>
#include <time.h>


LED_t led;
RTCZero mcuRtc;
struct tm currentTime;
volatile enum StateMachine step, nextAlarm;


void alarm()
{
	Serial.println("Alarm");
	step = nextAlarm;
}


void setup()
{
	led = { .r = 0, .g = 0 };
	
	pinMode(LED_R_PIN, OUTPUT);
	pinMode(LED_G_PIN, OUTPUT);
	pinMode(PWR_LED_PIN, OUTPUT);
	pinMode(USER_LED_PIN, OUTPUT);
	
	Serial.begin(DEBUG_BAUDRATE);
	Serial.setTimeout(1000);
	uint8_t serialStartupDelay = 50;
	while(!Serial && --serialStartupDelay)
	{
		delay(100);
	}
	Serial.println("\nHello!");
	
	time_t ntpTime = getNtpTime();

	mcuRtc.begin();
	mcuRtc.setEpoch(ntpTime);
	mcuRtc.setFrequencyCorrection(RTC_CALIB_VALUE);
	mcuRtc.attachInterrupt(alarm);
	mcuRtc.enableAlarm(mcuRtc.MATCH_HHMMSS);

	Serial.print("NTP time: ");
	char* cTime = ctime(&ntpTime);
	Serial.print(cTime); // ctime() seems to add some line break
	
	step = AFTER_STARTUP;
	if (isBetweenAlarms(ntpTime, BED_TIME, WAKEUP_TIME))
	{
		step = BED_TIME;
	}
	else if (isBetweenAlarms(ntpTime, WAKEUP_TIME, OFF_TIME))
	{
		step = WAKEUP_TIME;
	}
	
	setupNextAlarm();
}


void loop()
{
	switch(step)
	{
		case AFTER_STARTUP:
			delay(30 * 1000);
			step = IDLE;
			break;
		
		case IDLE:
			mcuRtc.standbyMode();
			break;
		
		case TIME_SYNC:
			updateRtcTime();
			setupNextAlarm();
			step = IDLE;
			break;
			
		case BED_TIME:
			if (led.g == 0)
			{
				step = BED_TIME_II;
				break;
			}
			--led.g;
			writeLED(&led);
			delay((1000 / BRIGHTNESS_STEPS));
			break;
		
		case BED_TIME_II:
			writeLED(&led);
			led.r++;
			delay((1000 / BRIGHTNESS_STEPS));
			if (led.r >= BRIGHTNESS_STEPS)
			{
				setupNextAlarm();
				step = IDLE;
			}
			break;
		
		case WAKEUP_TIME:
			if (led.r == 0)
			{
				step = WAKEUP_TIME_II;
				break;
			}
			--led.r;
			writeLED(&led);
			delay((1000 / BRIGHTNESS_STEPS));
			break;
		
		case WAKEUP_TIME_II:
			writeLED(&led);
			led.g++;
			delay((1000 / BRIGHTNESS_STEPS));
			if (led.g >= BRIGHTNESS_STEPS)
			{
				setupNextAlarm();
				step = IDLE;				
			}
			break;
		
		case OFF_TIME:
			if (led.r > 0) --led.r;
			if (led.g > 0) --led.g;
			writeLED(&led);
			delay((1000 / BRIGHTNESS_STEPS));
			if (led.r == 0 && led.g == 0)
			{
				setupNextAlarm();
				step = IDLE;
			}
			break;
	}
}


void setupNextAlarm()
{
	time_t rtc = mcuRtc.getEpoch();
	struct tm* rtcTime = gmtime(&rtc);
	
	long dt[ALARM_COUNT];
	for (uint8_t i = 0; i < ALARM_COUNT; i++)
	{
		dt[i] = calculateDeltaSeconds(&alarms[i].time, rtcTime);
	}
	
	// Search for alarms in the past, then shift them to the next day (add +24h)
	for (uint8_t i = 0; i < ALARM_COUNT; i++)
	{
		if (dt[i] < 0)
		{
			dt[i] += ONE_DAY;
		}
	}

	// Search for minimum
	long minDt = dt[0];
	uint8_t minIndex = 0;
	for (uint8_t i = 0; i < ALARM_COUNT; i++)
	{
		if (dt[i] < minDt)
		{
			minDt = dt[i];
			minIndex = i;
		}
	}
	const Alarm_t* pAlarm = &alarms[minIndex];
	
	char msg[50];
	sprintf(msg, "Next alarm: %s @ %02d:%02d:%02d\n", pAlarm->name, pAlarm->time.tm_hour, pAlarm->time.tm_min, pAlarm->time.tm_sec);
	Serial.print(msg);
	
	mcuRtc.setAlarmTime(pAlarm->time.tm_hour, pAlarm->time.tm_min, pAlarm->time.tm_sec);
	nextAlarm = pAlarm->opState;
}


bool isBetweenAlarms(const time_t now, const enum StateMachine alarm1, const enum StateMachine alarm2)
{
	time_t t1, t2;
	for (uint8_t i = 0; i < ALARM_COUNT; i++)
	{
		if (alarms[i].opState == alarm1)
		{
			t1 = makeFullAlarmTime(now, &alarms[i].time);
		}
		else if (alarms[i].opState == alarm2)
		{
			t2 = makeFullAlarmTime(now, &alarms[i].time);
		}
	}
	// Alarm2 is expected to always be after Alarm1, if not, add one day to shift it correctly.
	if (t2 < t1)
	{
		t2 += ONE_DAY;
	}
	return (now > t1 && now < t2);
}


void updateRtcTime()
{
	time_t ntpTime = getNtpTime();
	if (ntpTime != 0)
	{
		mcuRtc.setEpoch(ntpTime);
	}
}


time_t getNtpTime()
{
	UserLED(ON);
	if (connectWifi() != 0)
	{
		connectionError();
	}
	
#ifdef _DEBUG
	Serial.print("Get NTP time");
#endif
	uint8_t retries = 10;
	time_t newTime;	
	do
	{
		newTime = WiFi.getTime();
		if (newTime != 0)
		{
#ifdef _DEBUG
			Serial.println(" done.");
#endif
			WiFi.end();
			UserLED(OFF);
			return newTime;
		}
		delay(3 * 1000);
	}
	while (newTime == 0 && --retries > 0);

#ifdef _DEBUG
	Serial.println(" failed.");
#endif
	WiFi.end();
	UserLED(OFF);
	return 0;
}


uint8_t connectWifi()
{
	uint8_t retries = 10;
	while (WiFi.status() != WL_CONNECTED && --retries > 0)
	{
		Serial.println("Establishing WiFi connection ...");
		
		WiFi.setHostname(hostname);
		if (WiFi.begin(ssid, pass) == WL_CONNECTED)
		{
			Serial.print("Connected to '");
			Serial.print(ssid);
			Serial.print("'. IP: ");
			Serial.println(WiFi.localIP());
			
			WiFi.lowPowerMode();
			return 0;
		}
		
		WiFi.end();
		Serial.print("Cannot connect - Reason code: ");
		Serial.println(WiFi.reasonCode());
		
		delay(3 * 1000);
	}
	
	return 1;
}


void connectionError()
{
	while(1)
	{
		UserLED(OFF);
		delay(500);
		UserLED(ON);
		delay(500);
	}
}


long calculateDeltaSeconds(const struct tm* reference, const struct tm* current)
{
	// Assume that we're within the same day	
	long referenceSecsOfDay = reference->tm_hour * 3600 + reference->tm_min * 60 + reference->tm_sec;
	long currentSecsOfDay = current->tm_hour * 3600 + current->tm_min * 60 + current->tm_sec;
	return referenceSecsOfDay - currentSecsOfDay;
}


time_t makeFullAlarmTime(time_t rtcTime, const struct tm* alarmTime)
{
	// Alarm time only specifies hh:mm:ss, but for a correct calculation we need a correct time referring to the time epoch
	struct tm* actualAlarmTime = gmtime(&rtcTime);
	actualAlarmTime->tm_sec = alarmTime->tm_sec;
	actualAlarmTime->tm_min = alarmTime->tm_min;
	actualAlarmTime->tm_hour = alarmTime->tm_hour;
	return mktime(actualAlarmTime);	
}


void writeLED(const LED_t* led)
{
	const uint8_t brightnessCorrectionTable[BRIGHTNESS_STEPS] = { 0, 1, 2, 3, 4, 7, 11, 17, 28, 45, 69, 102, 141, 183, 255, 255 };
	analogWrite(LED_R_PIN, brightnessCorrectionTable[led->r]);
	analogWrite(LED_G_PIN, brightnessCorrectionTable[led->g]);
}
