///
/// @file	Communications.h 
/// @brief	Header
/// @details	<#details#>
/// @n	
/// @n @b	Project LEDPanel1
/// @n @a	Developed with [embedXcode](http://embedXcode.weebly.com)
/// 
/// @author	Matthias Hölling
/// @author	Matthias Hölling
/// @date	26.11.12 20:09
/// @version	<#version#>
/// 
/// @copyright	© Matthias Hölling, 2012
/// @copyright	CC = BY NC SA
///
/// @see	ReadMe.txt for references
///



#ifndef Communications_h
#define Communications_h

#include <SPI.h>
#include <Ethernet.h>
#include <avr/pgmspace.h>

#include "PanelData.h"

#define NTPSERVER 212, 101, 0, 10
#define NTP_PORT 8888
#define NTP_PACKET_SIZE 48
#define BUFFER_SIZE 128
#define RF_GROUP 212
#define RF_NODE 1
#define MOOD_NODE 2
#define MASTER_NODE 3



boolean ethernetInit();
boolean rfinit();
boolean isSummertime(time_t syncedTime);
void doWebService(PanelData* TheData);
void serialWriteWebPages();
void printXMLData(PanelData* PData);
void sendHTTPRequest(const char* string);
void _sendPageFromFlash(EthernetClient client, const char* pageString);
void _printPageFromFlash(const char* pageString);
void _sendXMLData(EthernetClient client, PanelData* PData);
void _doCommand(char* command, char* value, PanelData* Data);

static unsigned long _getNtpTime();
void _sendNTPpacket(IPAddress& address,byte* packetBuffer,EthernetUDP Udp);

uint8_t RF_MoodControl(byte* payload,uint8_t length, uint8_t status);
uint8_t RF_Probe();
uint8_t RF_sendToMaster(const char* string);





#endif
