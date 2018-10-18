#define USING_ESP_8266

#ifdef USING_ESP_8266
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#else
#include <SPI.h>
#include <Ethernet.h>
#include <EthernetUdp.h>
#endif

#include <TimeLib.h>
#include "SoftwareSerial.h"
#include "LoraWanForwarder.h"

/* Local config */
#ifdef USING_ESP_8266
char ssid[] = "YourSSID";
char pass[] = "YourPASS";
WiFiUDP Udp;
SoftwareSerial loraStream(12, 13);
#else
EthernetUDP Udp;
#define loraStream Serial1
#endif
LoraWanForwarder exLoraWan(&loraStream, 9600);
byte EthernetMac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xEA };

/* Server config */
#define LoraWanServerAddr "router.as2.thethings.network"
int LocalPort = 12345;
int LoraWanServerUpPort  = 1700;
int LoraWanServerDownPort = 1700;

/* LoraWan config */
#define FORWADER_FREQUENCY		433375000
#define FORWADER_DATARATE		0

void exLoraWanHandleIncommingMsg(unsigned char* msg) {
	int cmdID;
	int dataLength;
	unsigned char* data;

	cmdID = exLoraWan.getMsgCommandId(msg);
	data = exLoraWan.getMsgData(msg);
	dataLength = exLoraWan.getMsgLength(msg);

	exLoraWan.receivData(data, dataLength - 1);

}

void setup() {
	Serial.begin(9600);
	loraStream.begin(9600);

	Serial.println();
	Serial.println("Setting...");

#ifdef USING_ESP_8266
	WiFi.mode(WIFI_STA);
	WiFi.begin(ssid, pass);
	while (WiFi.status() != WL_CONNECTED) {
		delay(500);
		Serial.print(".");
	}
	Serial.println();
	Serial.println("WiFi connected");
	Serial.println("IP address: ");
	Serial.println(WiFi.localIP());
#else
	while (Ethernet.begin(EthernetMac) == 0) {
		Serial.println("Failed DHCP");
	}
	Serial.print("IP: ");
	IPAddress ip = Ethernet.localIP();
	for (byte i = 0; i < 4; i++) {
		Serial.print(ip[i], DEC);
		if (i < 3)  Serial.print(".");
		else Serial.println();
	}
#endif

	Udp.begin(LocalPort);
	setSyncProvider(getNtpTime);

	randomSeed(analogRead(0));
	exLoraWan.configMAC(EthernetMac);
	exLoraWan.configServer(&Udp, LoraWanServerAddr, LoraWanServerUpPort, LoraWanServerDownPort);
	if (!exLoraWan.configRadio(FORWADER_FREQUENCY, FORWADER_DATARATE)) {
		Serial.println("<-configRadio()");
		while (1) yield();
	}

	exLoraWan.incommingMsgHandlerRegister(exLoraWanHandleIncommingMsg);
	Serial.println("Done!");
}

void loop() {
	exLoraWan.update();
}

/* For sync time */
static const char *timeServer = "time.nist.gov";
static const int NTP_PACKET_SIZE = 48;
static byte packetBuffer[ NTP_PACKET_SIZE];

time_t getNtpTime() {
	while (Udp.parsePacket() > 0) ; // discard any previously received packets
	Serial.println("Transmit NTP Request");
	sendNTPpacket(timeServer);
	uint32_t beginWait = millis();
	while (millis() - beginWait < 1500) {
		int size = Udp.parsePacket();
		if (size >= NTP_PACKET_SIZE) {
			Serial.println("Receive NTP Response");
			Udp.read(packetBuffer, NTP_PACKET_SIZE);  // read packet into the buffer
			unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
			unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
			unsigned long secsSince1900 = highWord << 16 | lowWord;

			const unsigned long seventyYears = 2208988800UL;
			unsigned long epoch = secsSince1900 - seventyYears;
			return epoch;
		}
	}
	Serial.println("No NTP Response :-(");
	return now(); // return 0 if unable to get the time
}

// send an NTP request to the time server at the given address
void sendNTPpacket(const char* address) {
	// set all bytes in the buffer to 0
	memset(packetBuffer, 0, NTP_PACKET_SIZE);
	// Initialize values needed to form NTP request
	// (see URL above for details on the packets)
	packetBuffer[0] = 0b11100011;   // LI, Version, Mode
	packetBuffer[1] = 0;     // Stratum, or type of clock
	packetBuffer[2] = 6;     // Polling Interval
	packetBuffer[3] = 0xEC;  // Peer Clock Precision
	// 8 bytes of zero for Root Delay & Root Dispersion
	packetBuffer[12]  = 49;
	packetBuffer[13]  = 0x4E;
	packetBuffer[14]  = 49;
	packetBuffer[15]  = 52;
	// all NTP fields have been given values, now
	// you can send a packet requesting a timestamp:
	Udp.beginPacket(address, 123); //NTP requests are to port 123
	Udp.write(packetBuffer, NTP_PACKET_SIZE);
	Udp.endPacket();
}
