//
// LocalLibrary.cpp 
// Library C++ code
// ----------------------------------
// Developed with embedXcode 
// http://embedXcode.weebly.com
//
// Project LEDPanel1
//
// Created by Matthias Hölling, 11.11.12 20:14
// Matthias Hölling
//	
//
// Copyright © Matthias Hölling, 2012
// Licence CC = BY NC SA
//
// See LocalLibrary.cpp.h and ReadMe.txt for references
//

#include <Time.h>
#include <Color.h>
#include <SPI.h>
#include <Ethernet.h>
#include <EEPROM.h>
#include <string.h>

#include "LocalLibrary.h"
#include "PanelLight.h"
#include "PanelData.h"
#include "Communications.h"










/* Set RGB Values and Turn on and off the Light, Return RGB Vector for Web-Display */

void updatePanelDisplay(){
    
    static unsigned VCorrect=256;
    
    if(!TheData.doWhiteAdjust && (abs((int) VCorrect-(int) analogRead(senseLight))>10)) {
        VCorrect=analogRead(senseLight);
        //VCorrect=500;
        int Vtemp=VCorrect;
        if(VCorrect < 850) {
            Vtemp = VCorrect - (850 - VCorrect);
        }
        Vtemp = min(max(Vtemp/4,20),255);
        ThePanel.updateColor(Vtemp);
        //Serial.print("updated ambient light to "); Serial.print(Vtemp);
    }
    if (TheData.whiteNeedsChange()) {
        ThePanel.setWhiteOn();
        Serial.println("toggle white");
        int Vtemp=VCorrect;
        if(VCorrect < 850) {
            Vtemp = VCorrect - (850 - VCorrect);
        }
        Vtemp = min(max(Vtemp/4,20),255);
        if(!TheData.doWhiteAdjust) ThePanel.updateColor(Vtemp);
    }
    if (TheData.colorNeedsUpdate() && !TheData.doWhiteAdjust) {
        ThePanel.updateColor();
        if(TheData.moodLightOn(true)) TheData.moodColorUpdate=true;
    }
    
}

void updateClockDisplay(boolean update) {
    static int oldIndex = 0;
    static boolean whiteAdjust=false;
    if ((update && TheData.clockLightStateIndex==0 && !TheData.doWhiteAdjust) ||
         (TheData.clockLightStateIndex==0 && oldIndex!=0)) {
        if (oldIndex!=0) {
            Serial.println("resetting Clock");
            ThePanel.resetTime();
            ThePanel.displayLight(11, false);
            oldIndex =0;
        }

        
        
        Serial.println("updating Clock");
        ThePanel.displayTime();
    }
    else if (TheData.clockLightStateIndex==1 && oldIndex!=1) {
        ThePanel.displayLight(11, true);
        oldIndex=1;
        Serial.println("Turn clock on");
    }
    else if (TheData.clockLightStateIndex==2 && oldIndex !=2) {
        ThePanel.displayLight(11, false);
        oldIndex = 2;
        Serial.println("Turn clock off");
    }
    if (TheData.doWhiteAdjust) {
        if (!whiteAdjust) {
            whiteAdjust=true;
            Serial.println("update White vals");
            ThePanel.setPinsWhite();
            ThePanel.displayLight(11, true);
        }
        else if (TheData.whiteChanged) {
            Serial.println("really update White vals");
            ThePanel.setPinsWhite();
            TheData.whiteChanged=false;
        }
    }
    else if (whiteAdjust && !TheData.doWhiteAdjust) {
        whiteAdjust=false;
        Serial.println("stop updateing White vals");
        TheData.whiteChanged=false;
        ThePanel.updateColor();
        // if(TheData.moodLightStateIndex==1) TheData.moodColorUpdate=true;
        oldIndex=1;  //as if all lights had been on
        
    }
    
}



int seasonIndex(time_t newDay) {
    
    int months[12] = {31,31,28,31,30,31,30,31,31,30,31,30}; //starting in Dec
    int daysSinceDec1 =0 ;
    int deltDays = 0;
    int retVal;
    
    for(int m=0;m<(month(newDay)%12);m++)
        daysSinceDec1 += months[m];
    daysSinceDec1+=day(newDay)-1;
    Serial.print("month/day: ");Serial.print(month(newDay));Serial.print("/"); Serial.println(day(newDay));
    Serial.print("YTD: ");Serial.println(daysSinceDec1);
    
    
    if(month(newDay)>=12 || month(newDay)<3) //Dec - Feb
    {
        retVal = 3;
        deltDays = daysSinceDec1;
    }
    else if(month(newDay)<6) // Mar - May
    {
        retVal = 0;
        deltDays = 90*(daysSinceDec1-90)/92;
    }
    else if(month(newDay)<9) // Jun-Aug
    {
        retVal = 1;
        deltDays = 90*(daysSinceDec1-182)/92;
    }
    else  // Sep-Nov
    {
        retVal = 2;
        deltDays = 90*(daysSinceDec1-274)/91;
    }
    retVal += (deltDays<<2);
    return retVal;

}

/*-------- NTP code ----------*/



