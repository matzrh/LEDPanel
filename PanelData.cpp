//
// PanelData.cpp 
// C++ code
// ----------------------------------
// Developed with embedXcode 
// http://embedXcode.weebly.com
//
// Project LEDPanel1
//
// Created by Matthias Hölling, 26.11.12 22:45
// Matthias Hölling
//	
//
// Copyright © Matthias Hölling, 2012
// Licence CC = BY NC SA
//
// See PanelData.cpp.h and ReadMe.txt for references
//



#include "PanelData.h"
#include "Color.h"
#include "EEPROM.h"
#include "string.h"
#include <stdlib.h>

const char* PanelData::_backLightState[4] = {"Switch","On","Off","dead"};
const char* PanelData::_clockLightState[3] = {"Clock","On","Off"};
const unsigned PanelData::_initialOffWhite[3] = {RGBWHITE};


PanelData::PanelData() {
}

void PanelData::init() {
    SeasonColors[0]=Color(SPRING);
    SeasonColors[1]=Color(SUMMER);
    SeasonColors[2]=Color(FALL);
    SeasonColors[3]=Color(WINTER);
    _PresentColor = Color(0,0);
    LowerLimit = Color(0,0);
    UpperLimit = Color(0,0);
    for(int i=0;i<3;i++)
        _RGBWhite[i] = _initialOffWhite[i];
    Color::setWhite(_RGBWhite);
    _whiteOn = false;
    _whiteNotUpdated=false;
    _colorNotUpdated=false;
    _moodNotUpdated=false;
    backLightStateIndex=0;
    clockLightStateIndex=0;
    moodLightStateIndex=2;
    doWhiteAdjust=false;
    whiteChanged=false;
    scanner=true;
    moodColorUpdate=false;
}

void PanelData::switchOn(boolean switcher) {
    if (switcher != _whiteOn) {
        _whiteOn=switcher;
        _whiteNotUpdated=true;
        _moodNotUpdated = (moodLightStateIndex==0);
        //Serial.println(_moodNotUpdated ? "mood to update" : "no mood update");
    }
}

boolean PanelData::whiteNeedsChange() const {
    return _whiteNotUpdated;
}

boolean PanelData::moodNeedsUpdate() const {
    return _moodNotUpdated;
}

void PanelData::clearMoodUpdate() {
    _moodNotUpdated=false;
}

void PanelData::setWhiteIntensity(unsigned intensity) {
    _whiteIntensity = intensity;
    _whiteNotUpdated=true;
}

void PanelData::setMoodIntensity(unsigned intensity) {
    _moodIntensity = intensity;
    _moodNotUpdated = true;
}

unsigned PanelData::getMoodIntensity() const {
    return _moodIntensity;
}

    
Color PanelData::getPresentColor() const {
    return _PresentColor;
}

void PanelData::setColorLimits(int colorIndex) {
    int hueBefore, hueNext, satBefore, satNext;
    
    hueNext = (int) SeasonColors[((colorIndex&3)+1)%4].getHue();
    hueBefore = (int) SeasonColors[colorIndex&3].getHue();

    satNext = (int) SeasonColors[((colorIndex&3)+1)%4].getSat();
    satBefore = (int) SeasonColors[colorIndex&3].getSat();
    int delta = colorIndex>>2;
    int hueDir = hueNext-hueBefore;
    if(abs(hueDir>180)) {
        if (hueDir>0) hueDir -= 360;
        else hueDir+=360;
    }
    hueNext= hueBefore + hueDir*delta/90;
    hueNext += 360; hueNext%=360;
    satNext = satBefore + (satNext-satBefore)*delta/90;
    unsigned hueMin= (unsigned)(hueNext+180+abs(180-4*delta));  hueMin%=360;
    hueMin = (hueMin / 5) * 5;
    unsigned hueMax = (unsigned)(hueNext+180-abs(180-4*delta)); hueMax%=360;
    hueMax = (hueMax / 5) *5;
    unsigned satMin = max(0,satNext-45+abs(45-delta));
    satMin = (satMin / 10) * 10;
    unsigned satMax = min(100,satNext + 45 - abs(45-delta));
    satMax = (satMax / 10) * 10;
    LowerLimit.setHue(hueMin); LowerLimit.setSat(satMin);
    UpperLimit.setHue(hueMax); UpperLimit.setSat(satMax);
}

void PanelData::setPresentColorInLimits() {
    unsigned hue;
    if(LowerLimit.getHue()>UpperLimit.getHue())
        hue = ((unsigned) random(LowerLimit.getHue(), UpperLimit.getHue()+360))%360;
    else
        hue= (unsigned) random(LowerLimit.getHue(), UpperLimit.getHue());
    unsigned sat= (unsigned) random(LowerLimit.getSat(), UpperLimit.getSat());
    _PresentColor.setHue((hue/5)*5);
    _PresentColor.setSat((sat/10)*10);
    _colorNotUpdated = true;
}

void PanelData::fadePresentColorInLimits(){
    static boolean upwardsHue=true;
    static boolean upwardsSat=true;
    if (random(1,6) > 4) {  // about a fifth sat changes, hue otherwise
        unsigned int probeSat = _PresentColor.getSat() + (upwardsSat ? 1:-1);
        if ((probeSat<=UpperLimit.getSat()) && (probeSat>=LowerLimit.getSat())) {
            setPresentSat(probeSat);
        }
        else
            upwardsSat=!upwardsSat;
    }
    else {
        int probeHue =(int) _PresentColor.getHue() + (upwardsHue ? 1:-1);
        probeHue = (probeHue + 360) % 360;
        if (LowerLimit.getHue()>UpperLimit.getHue()) {
            if ((probeHue<=UpperLimit.getHue()) || (probeHue>=LowerLimit.getHue())) {
                setPresentHue((unsigned int) probeHue);
            }
            else
                upwardsHue=!upwardsHue;
        }
        else {
            if ((probeHue<=UpperLimit.getHue()) && (probeHue>=LowerLimit.getHue())) {
                setPresentHue(probeHue);
            }
            else
                upwardsHue=!upwardsHue;
        }
    }
}

void PanelData::setPresentColorManual(unsigned hue, unsigned sat) {
    _PresentColor.setHue(hue);
    _PresentColor.setSat(sat);
    _colorNotUpdated = true;
}

void PanelData::setPresentHue(unsigned int hue) {
    _PresentColor.setHue(hue);
    _colorNotUpdated = true;
}

void PanelData::setPresentSat(unsigned int sat) {
    _PresentColor.setSat(sat);
    _colorNotUpdated = true;
}

boolean PanelData::colorNeedsUpdate() {
    return _colorNotUpdated;
}

unsigned int PanelData::getOffWhite(int RGBIndex) {
    return _RGBWhite[RGBIndex];
}

void PanelData::setOffWhite(unsigned value, int index) {
    _RGBWhite[index]=min(255,value);
    whiteChanged=true;
     
}

void PanelData::setBackLight(char* value) {
    for(int i=0;i<3;i++) {
        if (strcmp(value, _backLightState[i])==0) {
            backLightStateIndex = i;
            _whiteNotUpdated = true;
        }
    }
}

boolean PanelData::getWhiteOn() {
    return ((backLightStateIndex==1) || (backLightStateIndex==0 && _whiteOn));
}

boolean PanelData::moodLightOn(boolean falseOverride) {
    return (falseOverride && ((moodLightStateIndex==1) || (_whiteOn && moodLightStateIndex==0)));
}

void PanelData::setClockLight(char * value) {
    Serial.println("In clocklight");
    for(int i=0;i<3;i++) {
        if (strcmp(value, _clockLightState[i])==0) {
            clockLightStateIndex = i;
        }
    }
    
}

void PanelData::setMoodLight(char* value) {
    for (int i=0; i<3; i++) {
        if (strcmp(value, _backLightState[i])==0)  {
            moodLightStateIndex = i;
            _moodNotUpdated = true;
        }
    }
}

char* PanelData::getXMLData(int index) {
    static char xmlString[MAXXMLSIZE]="";
    char valString[7]="";
    *xmlString = '\0';
    switch (index) {
        case 0:
            strcat(xmlString, "<?xml version=\"1.0\"?>\r\n<Data>\r\n");
            break;
        case 1:
            itoa(_PresentColor.getHue()/5,valString,10);
            strcat(xmlString, "<CurrentHueIndex>"); strcat(xmlString, valString); strcat(xmlString, "</CurrentHueIndex>\r\n");
            itoa(_PresentColor.getSat(), valString, 10);
            strcat(xmlString, "<CurrentSatIndex>"); strcat(xmlString, valString); strcat(xmlString, "</CurrentSatIndex>\r\n");
            break;
        case 2:
            itoa(SeasonColors[0].getHue()/5, valString, 10);
            strcat(xmlString, "<SpringHueIndex>"); strcat(xmlString, valString); strcat(xmlString, "</SpringHueIndex>\r\n");
            itoa(SeasonColors[0].getSat(), valString, 10);
            strcat(xmlString, "<SpringSatIndex>"); strcat(xmlString, valString); strcat(xmlString, "</SpringSatIndex>\r\n");
            break;
        case 3:
            itoa(SeasonColors[1].getHue()/5, valString, 10);
            strcat(xmlString, "<SummerHueIndex>"); strcat(xmlString, valString); strcat(xmlString, "</SummerHueIndex>\r\n");
            itoa(SeasonColors[1].getSat(), valString, 10);
            strcat(xmlString, "<SummerSatIndex>"); strcat(xmlString, valString); strcat(xmlString, "</SummerSatIndex>\r\n");
            break;
        case 4:
            itoa(SeasonColors[2].getHue()/5, valString, 10);
            strcat(xmlString, "<FallHueIndex>"); strcat(xmlString, valString); strcat(xmlString, "</FallHueIndex>\r\n");
            itoa(SeasonColors[2].getSat(), valString, 10);
            strcat(xmlString, "<FallSatIndex>"); strcat(xmlString, valString); strcat(xmlString, "</FallSatIndex>\r\n");
            break;
        case 5:
            itoa(SeasonColors[3].getHue()/5, valString, 10);
            strcat(xmlString, "<WinterHueIndex>"); strcat(xmlString, valString); strcat(xmlString, "</WinterHueIndex>\r\n");
            itoa(SeasonColors[3].getSat(), valString, 10);
            strcat(xmlString, "<WinterSatIndex>"); strcat(xmlString, valString); strcat(xmlString, "</WinterSatIndex>\r\n");
            break;
        case 6:
            strcat(xmlString, "<BackLightSelect>"); strcat(xmlString, _backLightState[backLightStateIndex]); strcat(xmlString, "</BackLightSelect>\r\n");
            itoa(_whiteIntensity, valString, 10);
            strcat(xmlString, "<BackLightValue>"); strcat(xmlString, valString); strcat(xmlString, "</BackLightValue>\r\n");
            break;
        case 7:
            strcat(xmlString, "<ClockSelect>"); strcat(xmlString, _clockLightState[clockLightStateIndex]); strcat(xmlString, "</ClockSelect>\r\n");
            break;
        case 8:
            strcat(xmlString, "<radiomood>"); strcat(xmlString, _backLightState[min(moodLightStateIndex,2)]); strcat(xmlString, "</radiomood>\r\n");
            itoa(_moodIntensity, valString, 10);
            strcat(xmlString, "<mood_slider>"); strcat(xmlString, valString); strcat(xmlString, "</mood_slider>\r\n");
            break;
        case 9:
            itoa(LowerLimit.getHue()/5,valString,10);
            strcat(xmlString, "<HueLimit1>"); strcat(xmlString, valString); strcat(xmlString, "</HueLimit1>\r\n");
            itoa(UpperLimit.getHue()/5,valString,10);
            strcat(xmlString, "<HueLimit2>"); strcat(xmlString, valString); strcat(xmlString, "</HueLimit2>\r\n");
            break;
        case 10:
            itoa(LowerLimit.getSat(),valString,10);
            strcat(xmlString, "<SatLimit>"); strcat(xmlString, valString); strcat(xmlString, "</SatLimit>\r\n");
            break;
        case 11:
            strcat(xmlString, "<ScanColors>"); strcat(xmlString, (scanner ? "scanned":"fixed")); strcat(xmlString, "</ScanColors>\r\n");
            break;
        case 12:
            itoa(_RGBWhite[0],valString,10);
            strcat(xmlString, "<RWhite>"); strcat(xmlString, valString); strcat(xmlString, "</RWhite>\r\n");
            itoa(_RGBWhite[1],valString,10);
            strcat(xmlString, "<GWhite>"); strcat(xmlString, valString); strcat(xmlString, "</GWhite>\r\n");
            break;
        case 13:
            itoa(_RGBWhite[2],valString,10);
            strcat(xmlString, "<BWhite>"); strcat(xmlString, valString); strcat(xmlString, "</BWhite>\r\n");
            
            strcat(xmlString, "</Data>\r\n");            
            break;
        default:
            *xmlString=0 ;
            break;
    }
    return xmlString;
}

void PanelData::saveEEPROM()  const {
    struct {
    void saveValue(int saveVal, int startAddress) {
        if(EEPROM.read(startAddress) != (saveVal & 0xff)) EEPROM.write(startAddress,saveVal & 0xff);
        if(EEPROM.read(startAddress+1) != (saveVal & 0xff00)>>8) EEPROM.write(startAddress+1,(saveVal & 0xff00)>>8);
    }
    void saveByte(uint8_t saveByte,int startAddress) {
            if(EEPROM.read(startAddress) != (saveByte)) EEPROM.write(startAddress, saveByte);
    }
    } writer;
    writer.saveValue(_whiteIntensity, 2);
    writer.saveValue(_PresentColor.getHue(), 4);
    writer.saveValue(_PresentColor.getSat(), 6);
    for (int i=8; i<24; i+=4) {
        writer.saveValue(SeasonColors[(i-8)/4].getHue(), i);
        writer.saveValue(SeasonColors[(i-8)/4].getSat(), i+2);
    }
    for (int i=24; i<27; i++) {
        Serial.print("Saved to address: "); Serial.println(i);
        writer.saveByte((uint8_t) _RGBWhite[i-24],i);
    }
    writer.saveByte(uint8_t (scanner? 1:0) ,27);
    writer.saveValue(_moodIntensity,28);
    writer.saveByte(backLightStateIndex,30);
    writer.saveByte(moodLightStateIndex,31);
}

void PanelData::loadEEPROM() {
    struct {
    int readValue(int startAddress) {
        int retVal = EEPROM.read(startAddress);
        retVal += EEPROM.read(startAddress+1)<<8;
        return retVal;
    }
    uint8_t readByte(int startAddress) {
        return EEPROM.read(startAddress);
    }
    } reader;
    _whiteIntensity = (unsigned) reader.readValue(2);
    _PresentColor.setHue((unsigned) reader.readValue(4));
    _PresentColor.setSat((unsigned) reader.readValue(6));
    for (int i=8; i<24; i+=4) {
        SeasonColors[(i-8)/4].setHue((unsigned) reader.readValue(i));
        SeasonColors[(i-8)/4].setSat((unsigned) reader.readValue(i+2));
    }
    for (int i=24; i<27; i++) {
        Serial.print("Read from address: "); Serial.println(i);
        _RGBWhite[i-24] = (unsigned) reader.readByte(i);
    }
    scanner = (reader.readByte(27)==1);
    _moodIntensity = (unsigned) reader.readValue(28);
    backLightStateIndex = (uint8_t) reader.readByte(30);
    if (backLightStateIndex>2) {
        backLightStateIndex=0;
    }
    moodLightStateIndex = min((uint8_t) reader.readByte(31),2);

}
