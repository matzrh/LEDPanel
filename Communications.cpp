//
// Communications.cpp 
// C++ code
// ----------------------------------
// Developed with embedXcode 
// http://embedXcode.weebly.com
//
// Project LEDPanel1
//
// Created by Matthias Hölling, 04.12.12 20:44
// Matthias Hölling
//	
//
// Copyright © Matthias Hölling, 2012
// Licence CC = BY NC SA
//
// See Communications.cpp.h and ReadMe.txt for references
//

/*
 * THE RF12 JEELIB LIBRARY NEEDS TO BE CHANGED TO USE PIN 21 AS INTTERRUPT.  FURTHERMORE, THE INT NUMBERING IS FALSE, 21 IS ACTUALLY INT0, PIN 2 WOULD BE INT 4 :-/
 * THE THE ETHERNET LIBRARY NEEDS TO BE MODIFIED SO THAT INTERRUPTS ARE DISABLED DURING SPI COMMUNICATION, OTHERWISE, THIS IS NOT WORKING WITH RF12
 * PLEASE, APPLY PATCHES TO LIBRARIES BEFORE COMPILATION
 */

#include "Arduino.h"

#include <SPI.h>
#include <Ethernet.h>
#include "EthernetBonjour.h"
//#include <Dns.h>
#include <avr/pgmspace.h>

#include <string.h>
#include "Time.h"
#include "RF12.h"

#include "Communications.h"
#include "PanelData.h"

//variables

PROGMEM const char _homepage[] = {
#include "control.dmp"
		};

PROGMEM const char _header[] =
		"HTTP/1.1 200 OK\r\nServer: arduino\r\nCache-Control: no-store, no-cache, must-revalidate\r\nPragma: no-cache\r\nConnection: close";

PROGMEM const char _404page[] =
		"HTTP/1.1 404 Not Found\r\nServer: arduino\r\nContent-Type: text/html\r\n\r\n<html><head><title>Arduino Web Server - Error 404</title></head><body><h1>Error 404: Sorry, that page cannot be found!</h1></body>";

uint8_t _mac[] = { 0x90, 0xA2, 0xDA, 0x0D, 0x04, 0xE1 };
EthernetServer server(80);
const long timeZoneOffset = 3600L;
const char sendURL[] = "scripts/remote.cgi?";
const char remoteServer[] = "cubie2.local";
IPAddress remoteIP(192, 168, 1, 31);  // fixed network address...

boolean ethernetInit() {
	static int net_ok = 0;
	if (!net_ok) {
		if (Ethernet.begin(_mac) == 0)
			return 0;
		server.begin();
		EthernetBonjour.begin("lichtuhr");
		EthernetBonjour.addServiceRecord("lichtuhr webserver._http", 80,
				MDNSServiceTCP);
		Serial.println("Ethernet Service startet");
	}
	net_ok = 1;
	setSyncProvider(_getNtpTime);
	setSyncInterval(1200); // sync every 20 minutes
	long timer = millis();
	while ((millis() - timer < 5000) && timeStatus() == timeNotSet)
		;
	//wait for sync
	return ((timeStatus() != timeNotSet));

}

boolean rfinit() {
	uint8_t node = rf12_initialize(RF_NODE, RF12_433MHZ, RF_GROUP);
	return (node == RF_NODE);
}

uint8_t RF_MoodControl(byte *payload, uint8_t length, uint8_t status) {
	//Serial.print("in mood update status "); Serial.println(status);
	static uint8_t failprobe = 0;
	uint8_t recData = RF_Probe();
	if ((status >= 1) && (status <= 3) && (length > 0)) { // try to send for states 1-3
		if (rf12_canSend()) {
			failprobe = 0;
			uint8_t header = (MOOD_NODE & RF12_HDR_MASK) | RF12_HDR_ACK
					| RF12_HDR_DST; // send to specific Node and request ACK
			rf12_sendStart(header, payload, length);
			Serial.print("sending "); Serial.println(*payload);
			return 4;
		} else if (++failprobe > 5) {
			Serial.println("failprobe>5, could not send forever");
			return 5;
		}
	} else if (recData > 0 && recData <3) { //something received
		return (3 * (--recData));  // status is 0 when ACK rec, 3 when Q rec.
	}
	return status;  // return same state if nothing received and nothing sent
}


uint8_t RF_Probe() {
	if ((rf12_recvDone()) && (rf12_crc == 0)) {
		if ((rf12_data[0] = 'Q') && (rf12_len>0)) {
			//delay(30); // delay 10msec for linux receiver to switch to receive mode
			//rf12_data[0]=0;
			return 2;
		}
		else if ((rf12_hdr & RF12_HDR_CTL) && ((rf12_hdr & RF12_HDR_MASK) == MOOD_NODE ) ) {
			return 1;
		}
		return 3;
	}
	return 0;
}

uint8_t RF_sendToMaster(const char *str) {
	const byte *payload = (const byte*) str;
	uint8_t len = strlen(str);
	uint8_t i, sentOff;
	for (i = 0; i < 4; i++) {
		unsigned long timer = millis();
		sentOff = 0;
		// Serial.print("now trying to send: ");Serial.print(str);
		// Serial.print(" with header: "); Serial.print((MASTER_NODE & RF12_HDR_MASK) | RF12_HDR_ACK | RF12_HDR_DST);
		// Serial.print(" and length:"); Serial.println(len);
		while (!sentOff && (millis() - timer < 500)) {
			if (rf12_canSend()) {
				uint8_t header = ((MASTER_NODE & RF12_HDR_MASK) | RF12_HDR_ACK
						| RF12_HDR_DST);
				rf12_sendStart(header, payload, len);
				sentOff = 1;
			}
		}
		timer = millis();
		while (millis() - timer < 2000) {
			if ((rf12_recvDone()) && (rf12_crc == 0)) {
				if ((rf12_hdr& RF12_HDR_CTL) && ((rf12_hdr & RF12_HDR_MASK) == MASTER_NODE)) {
					Serial.println("rf ok, I'm good");
					return 1;
				}
				else
				Serial.print("received: "); Serial.println(rf12_hdr);
			}
		}
		Serial.print("sentoff?:");
		Serial.println(sentOff);
		Serial.println("no Ack received");
	}
	return 0;
}

unsigned long _getNtpTime() {
	/* apart from getting the ntp time, this also renews/maintains the DHCP Lease */

	static IPAddress timeServer(NTPSERVER); // soho.solnet.ch NTP server from ch.pool.ntp.org
	static byte statPacketBuffer[ NTP_PACKET_SIZE]; //buffer to hold incoming and outgoing packets
	// A UDP instance to let us send and receive packets over UDP
	static EthernetUDP statUdp;
	static boolean UdpInitialized = false;

	if (!UdpInitialized) {
		statUdp.begin(NTP_PORT);
		UdpInitialized = true;

	}

	Serial.println("syncing...");
	_sendNTPpacket(timeServer, statPacketBuffer, statUdp); // send an NTP packet to a time server

	// wait to see if a reply is available
	delay(1000);
	if (statUdp.parsePacket()) {
		// We've received a packet, read the data from it
		statUdp.read(statPacketBuffer, NTP_PACKET_SIZE); // read the packet into the buffer
		Ethernet.maintain();  // this renews, maintains the DHCP lease

		//the timestamp starts at byte 40 of the received packet and is four bytes,
		// or two words, long. First, esxtract the two words:

		unsigned long highWord = word(statPacketBuffer[40],
				statPacketBuffer[41]);
		unsigned long lowWord = word(statPacketBuffer[42],
				statPacketBuffer[43]);
		// combine the four bytes (two words) into a long integer
		// this is NTP time (seconds since Jan 1 1900):
		unsigned long secsSince1900 = highWord << 16 | lowWord;
		const unsigned long seventy_years = 2208988800UL - timeZoneOffset;
		unsigned long retTime = secsSince1900 - seventy_years + 1;

		return retTime + (isSummertime(retTime) ? 3600L : 0); //because I waited 1sec?

	}
	return 0; // return 0 if unable to get the time
}

// send an NTP request to the time server at the given address
void _sendNTPpacket(IPAddress &address, byte *packetBuffer, EthernetUDP Udp) {
	// set all bytes in the buffer to 0
	memset(packetBuffer, 0, NTP_PACKET_SIZE);
	// Initialize values needed to form NTP request
	// (see URL above for details on the packets)
	packetBuffer[0] = 0b11100011;   // LI, Version, Mode
	packetBuffer[1] = 0;     // Stratum, or type of clock
	packetBuffer[2] = 6;     // Polling Interval
	packetBuffer[3] = 0xEC;  // Peer Clock Precision
	// 8 bytes of zero for Root Delay & Root Dispersion
	packetBuffer[12] = 49;
	packetBuffer[13] = 0x4E;
	packetBuffer[14] = 49;
	packetBuffer[15] = 52;

	// all NTP fields have been given values, now
	// you can send a packet requesting a timestamp:
	Udp.beginPacket(address, 123); //NTP requests are to port 123
	Udp.write(packetBuffer, NTP_PACKET_SIZE);
	Udp.endPacket();
}

/* check if summer (daylight saving) or winter time */

boolean isSummertime(time_t syncedTime) {
	// true if we have summertime according to CET / CEST convention
	// syncedTime is TZ adjusted, but always standard (winter) time
	if (month(syncedTime) > 3 && month(syncedTime) < 10)
		// summertime from April through September
		return true;
	else {
		if (month(syncedTime) == 3
				&& (day(syncedTime) - weekday(syncedTime)) >= 24
				&& (hour(syncedTime) >= 2 || weekday(syncedTime) > 1))
			// or summertime after 2 am last Sunday of March, Sunday -> weekday() = 1
			return true;
		else {
			return month(syncedTime) == 10
					&& ((day(syncedTime) - weekday(syncedTime)) < 24
							|| (weekday(syncedTime) == 1 && hour(syncedTime) < 2));
			// or summertime before 2 am last Sunday of March
		}
	}
}

// Handle HTTP requests to IP
void doWebService(PanelData *PData) {
	//rf12_suspend();
	EthernetBonjour.run();
	EthernetClient client;
	client = server.available();
	char buffer[BUFFER_SIZE];
	char c;
	memset(buffer, 0, BUFFER_SIZE);
	if (client) {
		if (client.connected() && client.available()) { // read a row
			buffer[0] = client.read();
			buffer[1] = client.read();
			uint8_t bufindex = 2;
			// read the first line to determinate the request page
			while (buffer[bufindex - 2] != '\r' && buffer[bufindex - 1] != '\n'
					&& bufindex < BUFFER_SIZE) { // read full row and save it in buffer
				c = client.read();
				buffer[bufindex] = c;
				bufindex++;
			}
			if (strncmp(buffer, "GET ", 4) == 0) {
				if (strncmp(buffer, "GET / ", 6) == 0) {
					_sendPageFromFlash(client, _header);
					client.println("Content-Type: text/html\r\n");
					_sendPageFromFlash(client, _homepage);
				} else if (strncmp(buffer, "GET /data.xml ", 14) == 0) {
					_sendPageFromFlash(client, _header);
					client.println("Content-Type: text/xml\r\n");
					_sendXMLData(client, PData);
				} else if (strncmp(buffer, "GET /setval?", 12) == 0) {
					char *command;
					command = buffer + 12;
					char *value = strpbrk(command, "= "); // locate end of command
					if (*value == '=') {
						*value = '\0'; //terminate command char*
						value += 1;
						char *helper = strpbrk(value, " "); // locate end of value
						*helper = '\0'; //terminate helper !! buffer lost at this point!!
					} else
						*value = '\0'; //terminate command, no value
					_doCommand(command, value, PData);
					_sendPageFromFlash(client, _header);
					client.println("Content-Type: text/xml\r\n");
					_sendXMLData(client, PData);
				} else {
					_sendPageFromFlash(client, (char*) &_404page);
				}

			}

		}
		client.stop();
		//rf12_resume();
	}

}

void sendHTTPRequest(const char *command) {
	EthernetClient client;
	Serial.println("send HTTP Request");
	if (client.connect(remoteIP, 80)) {
		Serial.print("connected to ");
		Serial.println(remoteServer);
		client.print("GET /");
		client.print(sendURL);
		client.print(command);
		client.println(" HTTP/1.0");
		client.println();
		while(client.connected() && client.available()) {
			char c=client.read();
			Serial.print(c);
		}
		client.stop();
	} else {
		Serial.print("could not connect to ");
		Serial.println(remoteServer);
	}
}

void serialWriteWebPages() {
	Serial.print("HTML Header: ");
	Serial.println((long) (&_header));
	_printPageFromFlash(_header);
	Serial.println();
	Serial.print("HTML Mainpage: ");
	Serial.println((long) (&_homepage));
	_printPageFromFlash(_homepage);
	Serial.println();
	Serial.print("HTML 404: ");
	Serial.println((long) (&_404page));
	_printPageFromFlash(_404page);
	Serial.println();
}

void _sendPageFromFlash(EthernetClient client, const char *pageString) {
	int total = 0;
	int start = 0;
	char buffer[BUFFER_SIZE];
	int realLen = strlen_P(pageString);
	memset(buffer, 0, BUFFER_SIZE);

	while (total <= realLen) {
		// print content
		strncpy_P(buffer, (char*) (pageString + start), BUFFER_SIZE - 1);
		// Serial.println(total);
		client.print(buffer);

		// more content to print?
		total = start + BUFFER_SIZE - 1;
		start = start + BUFFER_SIZE - 1;

	}
	client.println();
}

void _printPageFromFlash(const char *pageString) {
	int total = 0;
	int start = 0;
	char buffer[BUFFER_SIZE];
	int realLen = strlen_P(pageString);
	Serial.println(realLen);
	memset(buffer, 0, BUFFER_SIZE);

	while (total <= realLen) {
		// print content
		strncpy_P(buffer, (char*) (pageString + start), BUFFER_SIZE - 1);
		Serial.print(buffer);

		// more content to print?
		total = start + BUFFER_SIZE - 1;
		start = start + BUFFER_SIZE - 1;

	}
	Serial.println();
}

void _sendXMLData(EthernetClient client, PanelData *PData) {
	int start = 0;
	char *buffer;
	do {
		buffer = PData->getXMLData(start++);
		client.print(buffer);
	} while (*buffer != 0);
	client.println();
}

void printXMLData(PanelData *PData) {
	int start = 0;
	char *buffer;
	do {
		buffer = PData->getXMLData(start++);
		Serial.println(start);
		Serial.print(buffer);
	} while (*buffer != 0);
	Serial.println();
}

void _doCommand(char *command, char *value, PanelData *Data) {
	int seasonIndex = -1;
	Serial.println(command);
	if (strncmp(command, "ClockSelect", 11) == 0) {
		Data->setClockLight(value);
	} else if (strncmp(command, "BackLightSelect", 15) == 0) {
		Data->setBackLight(value);
	} else if (strncmp(command, "BackLightValue", 14) == 0) {
		Data->setWhiteIntensity((unsigned) atoi(value));
	} else if (strncmp(command, "CurrentHueIndex", 15) == 0) {
		Data->setPresentHue((unsigned) atoi(value));
	} else if (strncmp(command, "CurrentSatIndex", 15) == 0) {
		Data->setPresentSat((unsigned) atoi(value));
	} else if (strncmp(command, "Spring", 6) == 0) {
		seasonIndex = 0;
		command += 6;
	} else if (strncmp(command, "Summer", 6) == 0) {
		seasonIndex = 1;
		command += 6;
	} else if (strncmp(command, "Fall", 4) == 0) {
		seasonIndex = 2;
		command += 4;
	} else if (strncmp(command, "Winter", 6) == 0) {
		seasonIndex = 3;
		command += 6;
	} else if (strncmp(command, "auto", 4) == 0) {
		Data->setPresentColorInLimits();
	} else if (strncmp(command, "white", 5) == 0) {
		Data->doWhiteAdjust = ((atoi(value) & 1) > 0);
	} else if (strncmp(command, "RWhite", 6) == 0) {
		Data->setOffWhite((unsigned) atoi(value), 0);
	} else if (strncmp(command, "GWhite", 6) == 0) {
		Data->setOffWhite((unsigned) atoi(value), 1);
	} else if (strncmp(command, "BWhite", 6) == 0) {
		Data->setOffWhite((unsigned) atoi(value), 2);
	} else if (strncmp(command, "ScanColors", 10) == 0) {
		Data->scanner = (strncmp(value, "scanned", 7) == 0);
	} else if (strncmp(command, "radiomood", 9) == 0) {
		Data->setMoodLight(value);
	} else if (strncmp(command, "mood_slider", 11) == 0) {
		Data->setMoodIntensity((unsigned) atoi(value));
	}

	if (seasonIndex >= 0) {
		if (strncmp(command, "HueIndex", 8) == 0) {
			Data->SeasonColors[seasonIndex].setHue((unsigned) atoi(value));
		} else if (strncmp(command, "SatIndex", 8) == 0) {
			Data->SeasonColors[seasonIndex].setSat((unsigned) atoi(value));
		}

	}

}

