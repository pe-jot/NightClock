#pragma once

// Board: D1 Mini Pro

#define LED_R_PIN					15
#define LED_G_PIN					12
#define LED_B_PIN					13
#define BRIGHTNESS_STEPS			16

#define WIFI_CONNECTION_TIMEOUT		10000

#define LOCAL_PORT					2390
#define NTP_PORT					123
#define NTP_SERVER_NAME 			"time.nist.gov"
#define NTP_PACKET_SIZE				48		/* NTP time stamp is in the first 48 bytes of the message */

#define DEBUG_BAUDRATE				74880	/* Default ESP8266 bootloader baud rate */

#define SECONDS_PER_MINUTE			60
#define SECONDS_PER_HOUR			(60 * SECONDS_PER_MINUTE)
#define SECONDS_PER_DAY				(24 * SECONDS_PER_HOUR)
