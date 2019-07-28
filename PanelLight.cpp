/*
  WebTime.cpp - Library for getting Time with Arduino from NTP Server.
  Created by Matthias Hoelling, Nov 2012.
  
*/
#include <SPI.h>
#include "Arduino.h"
#include "Color.h"
#include "PanelLight.h"
#include "PanelData.h"

const uint8_t PanelLight::_pinsRGB[3] = RGB_PINS;
uint8_t PanelLight::_pinsHour[5] = HOUR_PINS;
uint8_t PanelLight::_pinsMinute[6] = MINUTE_PINS;
const uint8_t PanelLight::_pinWhite = WHITE_PIN;

PanelLight::PanelLight(PanelData* PData) {
    _TheData = PData;
    _valOnDisplay = 0;
    _oldHour=0; _oldMinute=0;
}

void PanelLight::initPins() {
    pinMode(_pinWhite, OUTPUT);
    
    for(int i=0;i<3;i++) {
        pinMode(_pinsRGB[i], OUTPUT);
        pinMode(_pinsHour[i], OUTPUT);
        pinMode(_pinsMinute[i], OUTPUT);
    }
    for(int i=3;i<5;i++) {
        pinMode(_pinsHour[i], OUTPUT);
        pinMode(_pinsMinute[i], OUTPUT);
    }
    pinMode(_pinsMinute[5], OUTPUT);

}

void PanelLight::updateColor() {
    //Serial.println("colors updated");
    _setColorPins();
    _TheData->_colorNotUpdated=false;
}
void PanelLight::updateColor(unsigned vNew) {
    _valOnDisplay=_TheData->getWhiteOn()? _TheData->_whiteIntensity : vNew;
    _setColorPins();
    _TheData->_colorNotUpdated=false;
}


void PanelLight::setWhiteOn() {
    analogWrite(_pinWhite,_TheData->getWhiteOn() ? _TheData->_whiteIntensity : 0);
    _TheData->_whiteNotUpdated=false;
    Serial.print("White Intensity set to ");
    Serial.println(_TheData->getWhiteOn() ? _TheData->_whiteIntensity : 0 );
}

uint8_t PanelLight::getRGBPin(int i) {
  return _pinsRGB[i];
}
uint8_t PanelLight::getHourPin(int i) {
  return _pinsHour[i];
}
uint8_t PanelLight::getMinutePin(int i) {
  return _pinsMinute[i];
}

unsigned PanelLight::getValOnDisplay() {
  return _valOnDisplay;
}

void PanelLight::displayLight(unsigned int panelIndex, boolean sustain){
    
    if (panelIndex == 11 ) _allOn(HIGH);
    else if (panelIndex <5) digitalWrite(_pinsHour[panelIndex], HIGH);
    else digitalWrite(_pinsMinute[panelIndex-5], HIGH);
    if (!sustain) {
        delay(500);
        _allOn(LOW);
    }
}

void PanelLight::RGBTest() {
    //initialize pins to 0
    for (int i=0; i<3; i++) analogWrite(_pinsRGB[i], 0);
    // then each color mit val 150 for 0.5sec
    for(int color=0;color<3;color++) {
        analogWrite(_pinsRGB[color],150);
        _allOn(HIGH);
        delay(500);
        _allOn(LOW);
        analogWrite(_pinsRGB[color],0);
    }
}

void PanelLight::showError(int duration) {
  Color White = Color (0,0);
  setPinsWhite();
      for(int i=0;i<duration;i++) {
          digitalWrite(_pinsHour[0], HIGH);
          digitalWrite(_pinsMinute[5], HIGH);
          delay(500);
          digitalWrite(_pinsHour[0], LOW);
          digitalWrite(_pinsMinute[5], LOW);
          delay(500);
    }
}

void PanelLight::displayTime() {
  _fadeIn(_TheData->hourDisplayed,_oldHour,4,_pinsHour);
  _fadeIn(_TheData->minuteDisplayed,_oldMinute,5,_pinsMinute);
  _oldHour = _TheData->hourDisplayed; _oldMinute = _TheData->minuteDisplayed;
}

void PanelLight::resetTime() {
    _oldHour=0; _oldMinute=0;
}

void PanelLight::_allOn(uint8_t value) {
  //test colors with all on
  for(int i=0;i<5;i++) {
    digitalWrite(_pinsHour[i], value);
    digitalWrite(_pinsMinute[i], value);
  }
  digitalWrite(_pinsMinute[5], value);
}


void PanelLight::_setColorPins() {
  unsigned value = _whiteOn ? (_TheData->_whiteIntensity) : _valOnDisplay;
  analogWrite(_pinsRGB[0],_TheData->_PresentColor.getRedCorrected(value));
  analogWrite(_pinsRGB[1],_TheData->_PresentColor.getGreenCorrected(value));
  analogWrite(_pinsRGB[2],_TheData->_PresentColor.getBlueCorrected(value));
}

void PanelLight::setPinsWhite () {
    for(int i=0; i<3;i++)
        analogWrite(_pinsRGB[i], _TheData->_RGBWhite[i]);
}
  
void PanelLight::_fadeIn(unsigned digits, unsigned digBefore, int maxByte, uint8_t pins[]) {
  unsigned plusWord=digits & (~ digBefore);
  unsigned minusWord=(~ digits) & digBefore;
    Serial.print("new hour/minute"); Serial.println(digits);
    Serial.print("old hour/minute"); Serial.println(digBefore);
    // now lets get real:
  for(int i=0;i<=maxByte;i++){
      if(((0x01<<(maxByte-i)) & plusWord) != 0) {
      digitalWrite(pins[i],HIGH);
      Serial.print("wrote HIGH to Pin "); Serial.println(pins[i]);
      }
  }
  delay(300); // first turn on necessary panels, then off to make a cool effect
  for(int i=0;i<=maxByte;i++){
      if(((0x01<<(maxByte-i)) & minusWord) != 0) {
      digitalWrite(pins[i],LOW);
      Serial.print("wrote LOW to Pin "); Serial.println(pins[i]);
      }
  }
}
