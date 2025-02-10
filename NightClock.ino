#include <time.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include "credentials.h"
#include "config.h"


typedef struct LED_t {
	uint8_t r;
	uint8_t g;
	uint8_t b;	
} LED_t;

// UTC
const struct tm bedTime = { .tm_sec = 0, .tm_min = 0, .tm_hour = 17 };
const struct tm wakeupTime = { .tm_sec = 0, .tm_min = 0, .tm_hour = 5 };

unsigned char packetBuffer[NTP_PACKET_SIZE]; // buffer to hold incoming and outgoing packets
WiFiUDP udp; // A UDP instance to let us send and receive packets over UDP
LED_t led;
time_t timestamp;
struct tm currentTime;
struct tm bedLastCheck, wakeupLastCheck;
uint8_t step;


void setup()
{
	led = { .r = 0, .g = 0, .b = 0 };
	step = 99;
	
	pinMode(LED_R_PIN, OUTPUT);
	pinMode(LED_G_PIN, OUTPUT);
	pinMode(LED_B_PIN, OUTPUT);
	analogWrite(LED_B_PIN, 1);
	
	Serial.begin(DEBUG_BAUDRATE);
	Serial.setTimeout(1000);
	while (!Serial);
	Serial.println("\nHello!");
	
	// Delay is somehow necessary to successfully establish a WiFi connection after DeepSleep.
	delay(1);
	
	if (connectWifi() != 0)
	{
		connectionError();
	}
	
	timestamp = getNtpTime();
	gmtime_r(&timestamp, &bedLastCheck);
	gmtime_r(&timestamp, &wakeupLastCheck);
	Serial.printf("NTP time: %s", ctime(&timestamp));	// ctime() seems to add some line break
	
	analogWrite(LED_B_PIN, 0);
}


void loop()
{
	switch(step)
	{
		case 0:
			if (led.g == 0)
			{
				step++;
				break;
			}
			--led.g;
			writeLED(&led);
			delay((1000 / BRIGHTNESS_STEPS));
			break;
			
		case 1:
			writeLED(&led);
			led.r++;
			delay((1000 / BRIGHTNESS_STEPS));
			if (led.r >= BRIGHTNESS_STEPS) step = 99;
			break;

		case 10:
			if (led.r == 0)
			{
				step++;
				break;
			}
			--led.r;
			writeLED(&led);
			delay((1000 / BRIGHTNESS_STEPS));
			break;

		case 11:
			writeLED(&led);
			led.g++;
			delay((1000 / BRIGHTNESS_STEPS));
			if (led.g >= BRIGHTNESS_STEPS / 2) step = 99;
			break;
		
		case 99:
			delay(60 * 1000);
			
			timestamp = getNtpTime();
			gmtime_r(&timestamp, &currentTime);
			Serial.printf("NTP time: %s", ctime(&timestamp));	// ctime() seems to add some line break
			
			if (timeCrossed(&bedTime, &bedLastCheck, &currentTime))
			{
				Serial.println("Bed Time crossed!");
				step = 0;
			}
			else if (timeCrossed(&wakeupTime, &wakeupLastCheck, &currentTime))
			{
				Serial.println("Wakeup Time crossed!");
				step = 10;
			}
			break;
	}
}


void writeLED(const LED_t* led)
{
	const uint8_t brightnessCorrectionTable[BRIGHTNESS_STEPS] = { 0, 1, 2, 3, 4, 7, 11, 17, 28, 45, 69, 102, 141, 183, 255, 255 };
	analogWrite(LED_R_PIN, brightnessCorrectionTable[led->r]);
	analogWrite(LED_G_PIN, brightnessCorrectionTable[led->g]);
	analogWrite(LED_B_PIN, brightnessCorrectionTable[led->b]);
}


unsigned char connectWifi()
{
	if (!WiFi.mode(WIFI_STA) || !WiFi.begin(ssid, pass) || (WiFi.waitForConnectResult(WIFI_CONNECTION_TIMEOUT) != WL_CONNECTED))
	{
		WiFi.mode(WIFI_OFF);
		Serial.println("Cannot connect WiFi!");
		WiFi.printDiag(Serial);
		Serial.flush();
		return 1;
	}
	
	Serial.print("IP address: ");
	Serial.println(WiFi.localIP());
	
	return 0;
}


void connectionError()
{
	while(1)
	{
		analogWrite(LED_B_PIN, 0);
		delay(500);
		analogWrite(LED_B_PIN, 1);
		delay(500);
	}
}

time_t getNtpTime()
{
#ifdef _DEBUG
	Serial.print("Starting UDP ... ");
#endif
	udp.begin(LOCAL_PORT);
#ifdef _DEBUG
	Serial.printf("local port: %u\n", udp.localPort());
#endif
	
	char resendRetries = 6;
	while (resendRetries)
	{
		sendNtpRequest();
	
		// Wait for response (max. 100 x 100ms = 10s)
		int replyLength = 0;
		char receiveRetries = 100;
		do
		{
			delay(100);
			replyLength = udp.parsePacket();
		} while (replyLength == 0 && --receiveRetries > 0);
			
		if (replyLength == 0 && receiveRetries == 0)
		{
#ifdef _DEBUG
			Serial.println("No NTP reply received. Retrying...");
#endif
			--resendRetries;
			continue;	// Start sending a new request
		}
		
		// We've received a packet, read the data from it
		udp.read(packetBuffer, NTP_PACKET_SIZE);
		
		return parseNtpResponse(packetBuffer);
	}
	
	return (time_t)0;
}


// send an NTP request to the time server at the given address
void sendNtpRequest()
{
#ifdef _DEBUG
	Serial.println("Sending NTP packet...");
#endif
	
	IPAddress timeServerIp;
	// Don't hardwire the IP address or we won't get the benefits of the pool. Lookup the IP address for the host name instead.
	WiFi.hostByName(NTP_SERVER_NAME, timeServerIp);
	
	// set all bytes in the buffer to 0
	memset(packetBuffer, 0, NTP_PACKET_SIZE);
	// Initialize values needed to form NTP request
	// (see URL above for details on the packets)
	packetBuffer[0] = 0b11100011;	// LI, Version, Mode
	packetBuffer[1] = 0;			// Stratum, or type of clock
	packetBuffer[2] = 6;			// Polling Interval
	packetBuffer[3] = 0xEC;			// Peer Clock Precision
	// 8 bytes of zero for Root Delay & Root Dispersion
	packetBuffer[12]	= 49;
	packetBuffer[13]	= 0x4E;
	packetBuffer[14]	= 49;
	packetBuffer[15]	= 52;

	// all NTP fields have been given values, now
	// you can send a packet requesting a timestamp:
	udp.beginPacket(timeServerIp, NTP_PORT);
	udp.write(packetBuffer, NTP_PACKET_SIZE);
	udp.endPacket();
}


time_t parseNtpResponse(const byte* data)
{	
	// the timestamp starts at byte 40 of the received packet and is four bytes,
	// or two words, long. First, esxtract the two words:
	unsigned long highWord = word(data[40], data[41]);
	unsigned long lowWord = word(data[42], data[43]);
	// combine the four bytes (two words) into a long integer
	// this is NTP time (seconds since Jan 1 1900):
	unsigned long secsSince1900 = highWord << 16 | lowWord;

	// now convert NTP time into everyday time:
	// Unix time starts on Jan 1 1970. In seconds, that's 2208988800:
	const unsigned long seventyYears = 2208988800UL;
	// subtract seventy years:
	unsigned long epoch = secsSince1900 - seventyYears;
	
	return (time_t)epoch;
}


bool timeCrossed(const struct tm* reference, struct tm* previous, const struct tm* current)
{
	bool crossed = (calculateDeltaSeconds(reference, previous) < 0) && (calculateDeltaSeconds(reference, current) > 0);
	memcpy(previous, current, sizeof(tm));
	return crossed;
}


long calculateDeltaSeconds(const struct tm* reference, const struct tm* current)
{
	// Assume that we're within the same day	
	long referenceSecsOfDay = reference->tm_hour * 3600 + reference->tm_min * 60 + reference->tm_sec;
	long currentSecsOfDay = current->tm_hour * 3600 + current->tm_min * 60 + current->tm_sec;
	return currentSecsOfDay - referenceSecsOfDay;
}


void printTime(const struct tm* time)
{
	Serial.printf("%2d:%2d:%2d", time->tm_hour, time->tm_min, time->tm_sec);
}
