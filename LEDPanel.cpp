///
/// @mainpage	LEDPanel1
/// @details	<#details#>
/// @n
/// @n
/// @n @a	Developed with [embedXcode](http://embedXcode.weebly.com)
///
/// @author	Matthias Hölling
/// @author	Matthias Hölling
/// @date	11.11.12 20:14
/// @version	<#version#>
///
/// @copyright	© Matthias Hölling, 2012
/// @copyright	CC = BY NC SA
///
/// @see	ReadMe.txt for references
///

///
/// @file	LEDPanel1.ino
/// @brief	Main sketch
/// @details	<#details#>
/// @n @a	Developed with [embedXcode](http://embedXcode.weebly.com)
///
/// @author	Matthias Hölling
/// @author	Matthias Hölling
/// @date	11.11.12 20:14
/// @version	<#version#>
///
/// @copyright	© Matthias Hölling, 2012
/// @copyright	CC = BY NC SA
///
/// @see	ReadMe.txt for references
/// @n
///

// Core library
//   declared in main.cpp


#include "Arduino.h"
// Include application, user and local libraries
#include <Time.h>
#include "Color.h"
#include <SPI.h>
#include <Ethernet.h>
#include <EEPROM.h>

#include "LocalLibrary.h"
#include "PanelLight.h"
#include "PanelData.h"
#include "Communications.h"

#define EEPROMRESET 99

uint8_t oldDay = 0;  // day loaded from EEPROM

PanelData TheData = PanelData();
PanelLight ThePanel = PanelLight(&TheData);

void setup() {
	/*
	 * turn on serial communication
	 * put self sustaining pin on high
	 * set all other pins
	 */
	uint8_t rfok = 0;
	uint8_t i = 0;
	Serial.begin(115200);
	Serial.println("starting...");
	pinMode(selfOn, OUTPUT);
	digitalWrite(selfOn, HIGH);
	// setprobePin to Input and setup White output
	pinMode(probeSwitch, INPUT);
	digitalWrite(probeSwitch, HIGH); //set pullup resistor

	ThePanel.initPins();
	TheData.init();

	/* start Ethernet and UDP
	 * configure UDP
	 * configure time servers and sync mechanism
	 */
	if ((rfinit())) {
		delay(500);
		Serial.println("RF module initialized");
	}
	//Serial.print("EICRB ");Serial.println(EICRB,BIN);
	//Serial.print("EIMSK ");Serial.println(EIMSK,BIN);
	while (!ethernetInit()) {
		Serial.println("Ethernet Init failed..");
		ThePanel.showError(5);
		if ((!rfok) || (i++ == 4)) {
			Serial.println("Failed to configure Ethernet using DHCP");
			ThePanel.showError(30);
			digitalWrite(selfOn, LOW);
			// no point in carrying on, so do nothing forevermore:
			for (;;)
				;
		} else
			delay(5000); // if rf working, try in 10 secs again
	}
	Serial.println("DHCP and NTP ok");
	/* do some test routines
	 * first RGB
	 * then Winter, Spring, Summer, Fall
	 */
	sendHTTPRequest("on");
	ThePanel.RGBTest();
	for (int i = 0; i < 4; i++) {
		TheData.setColorLimits(i);
		TheData.setPresentHue(TheData.LowerLimit.getHue());
		TheData.setPresentSat(TheData.LowerLimit.getSat());
		ThePanel.updateColor(150);
		// some debugging
		ThePanel.displayLight(11, false);
		// end debugging
	}
	ThePanel.setPinsWhite();
	for (int i = 0; i < 12; i++)
		ThePanel.displayLight(i, false);

	//and test white reading light
	TheData.setWhiteIntensity(150);
	TheData.switchOn(true);
	ThePanel.setWhiteOn();
	delay(500);
	TheData.switchOn(false);
	ThePanel.setWhiteOn();

	// read values from EEPROM
	oldDay = EEPROM.read(0);
	if (EEPROM.read(EEPROMRESET) == 0xff) {
		TheData.saveEEPROM();
		EEPROM.write(EEPROMRESET, 0);

	} else {
		TheData.loadEEPROM();
	}
	TheData.setColorLimits(seasonIndex(now()));
	Serial.print("day read: ");
	Serial.println(oldDay);
	Serial.print("hue read: ");
	Serial.println(TheData.getPresentColor().getHue());
	Serial.print("sat read: ");
	Serial.println(TheData.getPresentColor().getSat());

	// serialWriteWebPages();
	// printXMLData(&TheData);
	Serial.print("EICRA ");
	Serial.println(EICRA, BIN);
	Serial.print("EIMSK ");
	Serial.println(EIMSK, BIN);
	Serial.print("MLSI");
	Serial.println(TheData.moodLightStateIndex);

}

/*
 * the stuff to run....
 */

void loop() {
	static boolean update = true;
	static unsigned int oldTenSeconds = 0;
	static boolean whiteOn = false;
	static uint8_t switchTimer = 0;
	static long tBefore = 0;  // timer for turn off
	static long tSend = 0;  // timer for RF acknowledgement
	static uint8_t failSend = 0;
	static boolean last_mood_http_notified = false;
	static uint8_t recState = 3;
	static uint8_t rfLen = 0;
	static uint8_t saveMoodState = 0;
	static byte *rfString;
	static byte payLoadString[10] = { 'H', 0, 0, 'S', 0, 'X', 2, 'I', 0, 0 }; // protocol: Hue, Sat, Switch, Intensity

	/* state machine for **switchTimer**:
	 @t=0 && switch activated:  start timer (save millis()), @t+1=1
	 @t=1 && 250ms passed: @t+1=2, set switch state locally and in PanelData
	 @t=2 && switch activated again && less than 3 sec passed:  write EEPROM Data, initiate RF transfer, @t+1=3
	 @t=2 && 3 sec passed: @t+1=0
	 @t=3 && RF transfer completed or definitely failed:  turn everything off

	 */

	if ((whiteOn ? LOW : HIGH) != digitalRead(probeSwitch)) { // switch was activated
		switch (switchTimer) {
		case 0:
			tBefore = millis();
			switchTimer++;
			Serial.print("timer start:");
			Serial.println(tBefore);
			break;
		case 2:
			if (whiteOn && (millis() - tBefore < 3000)) { // reliable switch on position changed again...., turn it off!
				switchTimer++;  //to 3
				Serial.println("Turn off");
				if (TheData.moodLightStateIndex == 3) {  // was dead before...
					Serial.println("reset moodstate");
					TheData.moodLightStateIndex = saveMoodState; // do not save state 3
				}

				EEPROM.write(0, (day() & 0xff));
				TheData.saveEEPROM();
				recState = 2;  // send RF signal to turn off light.

			}
			break;

		default:
			break;
		}
	}
	switch (switchTimer) {
	case 1:
		if (millis() - tBefore > 250) { // 250 ms passed since last switch
			switchTimer++;
			if (digitalRead(probeSwitch) == LOW) {  // reliably turned on
				TheData.switchOn(true);
				whiteOn = true;
				Serial.println("Switch is on");
			} else {  // reliably turned off
				TheData.switchOn(false);
				whiteOn = false;
			}
		}
		break;
	case 2:
		if (millis() - tBefore > 3000) { // nothing happened for 3 seconds since switch
			switchTimer = 0;
		}
		break;
	case 3:
		if ((recState == 0) || (recState == 5)) { // wait until peripheral RF signal confirmed before turning off or resent not successful
			sendHTTPRequest("off");
			delay(100);  // allow some time to send request
			digitalWrite(selfOn, LOW);
		}
		break;
	default:
		break;
	}

	if (minute() != TheData.minuteDisplayed) //update the display every minute, only
			{
		TheData.minuteDisplayed = minute();
		TheData.hourDisplayed = hour();
		Serial.println(month() * 100 + day());
		Serial.println(hour() * 100 + minute());
		update = true;
	}
	if (day() != oldDay) {
		TheData.setColorLimits(seasonIndex(now()));
		oldDay = day();
	}

	if (TheData.scanner && (second() / 10 != oldTenSeconds)) {
		oldTenSeconds = second() / 10;
		TheData.fadePresentColorInLimits();
		//Color tempCol=TheData.getPresentColor();
		//Serial.print(tempCol.getHue());Serial.print("/");Serial.println(tempCol.getSat());
		//Serial.println(TheData.colorNeedsUpdate() ? "ja" : "nein");
	}

	// RF & Web Server)

	/* state machine for **recState** RF transfer
	 @t=0: "@t"= moodUpdate | moodColorUpdate from PanelData, (immediately)
	 @t=0: tSend=0, failSend=0, rfString=NULL, @t+1=0 or @t+1=3 if request ('Q') received
	 @t=1: RFString=ColorUpdate, reset moodColorUpdate in PanelData @t+1=4 or @t+1=5 if no communication with RF unit after 5 trials
	 @t=2: RFstring=moodUpdate, reset moodUpdate in Paneldata @t+1=(same as for @t=1)
	 @t=3: RFSting = mood and colorupdat, reset both values in PanelData, @t+1=(same as for @t=1)
	 @t=4 && timer not running/cleared: start timer @t+1=4 or @t+1=0 if ACK received
	 @t=4 && between 1.5 and 2 sec passed: clear timer, increase failsend,
	 failsend >3: clear moodUpdate, reset failsend, @t+1=5
	 failsend <=3: @t+1=3 (resend everything)
	 obsolete: @t=5 (dead): moodlight-> dead, @t+1=5
	 */

	if (recState == 0) { // if idle, trigger necessary updates... otherwise, finish what you are doing
		recState = (TheData.moodNeedsUpdate() ? 2 : 0)
				+ (TheData.moodColorUpdate ? 1 : 0);
		if (recState > 0) {
			Serial.print("RF update ");
			Serial.println(recState);
		}
	}
	switch (recState) {
	case 0:  // idle, nothing sending, test different vars
		if (tSend > 0)
			tSend = 0;
		if (failSend > 0)
			failSend = 0;
		rfLen = 0;
		rfString = 0;
		break;
	case 1: // send color only
		rfLen = 5; //assign and send only stuff before 'X'
		rfString = payLoadString;
		rfString[1] = (TheData.getPresentColor().getHue() & 0xff00) >> 8; // hue can be > 255
		rfString[2] = (TheData.getPresentColor().getHue() & 0xff);
		rfString[4] = TheData.getPresentColor().getSat();
		TheData.moodColorUpdate = false;
		//Serial.println("state 1");
		break;
	case 2: //send mood controls only
		rfLen = 4;
		rfString = &payLoadString[5]; //assign and send starting from 'X'
		rfString[1] = (byte) (TheData.moodLightOn(switchTimer != 3) ? 1 : 2);
		rfString[3] = (byte) TheData.getMoodIntensity();
		if (last_mood_http_notified != TheData.moodLightOn(switchTimer != 3)) {
			sendHTTPRequest(
					TheData.moodLightOn(switchTimer != 3) ?
							"moodon" : "moodoff");
			last_mood_http_notified = TheData.moodLightOn(switchTimer != 3);
		}
		TheData.clearMoodUpdate();
		//Serial.println("state 2");
		break;
	case 3:  //send color and mood controls
		rfLen = 9; //send the whole string
		rfString = payLoadString;
		rfString[1] = (TheData.getPresentColor().getHue() & 0xff00) >> 8; // hue can be > 255
		rfString[2] = (TheData.getPresentColor().getHue() & 0xff);
		rfString[4] = TheData.getPresentColor().getSat();
		rfString[6] = (byte) (TheData.moodLightOn(switchTimer != 3) ? 1 : 2);
		rfString[8] = (byte) TheData.getMoodIntensity();
		if (last_mood_http_notified != TheData.moodLightOn(switchTimer != 3)) {
			sendHTTPRequest(
					TheData.moodLightOn(switchTimer != 3) ?
							"moodon" : "moodoff");
			last_mood_http_notified = TheData.moodLightOn(switchTimer != 3);
		}
		TheData.clearMoodUpdate();
		TheData.moodColorUpdate = false;
		break;
	case 4: //waiting for acknowledgement
		//Serial.print(tSend);Serial.print(" ");Serial.println(millis()-tSend);
		if (tSend == 0) {
			tSend = millis();
		} else if ((millis() - tSend > 1500) && (millis() - tSend < 2000)) { //1.5-2 sec for ACK -> failed
			tSend = 0;
			Serial.print("No Ack, failsend count:");
			Serial.println(failSend);
			if (failSend++ > 3) {
				TheData.clearMoodUpdate();
				failSend = 0;
				saveMoodState = min(TheData.moodLightStateIndex, 2); // do not save index 3 or restore will fail
				recState = 0; //reset everything.  Give up on sending
			} else {
				recState = 3; // send all again
				Serial.println("resending ");
			}
		}
		rfLen = 0;
		rfString = 0;
		//Serial.println("waiting");
		break;
	default: //happens when recState is 5 -> dead
		//rfLen=0; rfString=0;
		//TheData.moodLightStateIndex=3;
		break;
	}
	recState = RF_MoodControl(rfString, rfLen, recState);



	doWebService(&TheData);

	updatePanelDisplay();
	updateClockDisplay(update);
	update = false;

}

