///
/// @file	LocalLibrary.h 
/// @brief	Library header
/// @details	<#details#>
/// @n	
/// @n @b	Project LEDPanel1
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



#ifndef LEDPanel1_LocalLibrary_h
#define LEDPanel1_LocalLibrary_h

#include <Color.h>
#include "PanelLight.h"
#include "PanelData.h"



// Variables and constants for time keeping
const long timeZoneOffset = 3600L; // set this to the offset in seconds to your local time;

// Variables and constants for RGB pin control, declaration only
const int probeSwitch=40;
const int senseLight=A0;
const int selfOn=19;

extern uint8_t oldDay;  // day loaded from EEPROM

extern PanelLight ThePanel;
extern PanelData TheData;

int seasonIndex(time_t newDay);
void updatePanelDisplay();
void updateClockDisplay(boolean update);

#endif
