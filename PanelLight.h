/*
  PanelLight.h - Library for Diplay on Panels.
  Created by Matthias Hoelling, Nov 2012.
  
*/
#ifndef PanelLight_h
#define PanelLight_h

#include "Arduino.h"
#include "Color.h"
#include "PanelData.h"

#define RGB_PINS {3,5,6}
#define MINUTE_PINS {22,28,26,24,23,25}
#define HOUR_PINS {38,36,32,34,30}
#define WHITE_PIN 7


class PanelLight
{
  public:
    PanelLight(PanelData* PData);
    void initPins();
    void updateColor();
    void updateColor(unsigned vNew);
    void setWhiteOn();
    void setPinsWhite();
    unsigned getValOnDisplay();
    void displayLight(unsigned int panelIndex, boolean sustain);
    uint8_t getRGBPin(int i);
    uint8_t getHourPin(int i);
    uint8_t getMinutePin(int i);
    void RGBTest();
    void showError(int duration);
    void displayTime();
    void resetTime();
  
    
  private:
    static const uint8_t _pinsRGB[3];
    static uint8_t _pinsMinute[6];
    static uint8_t _pinsHour[5];
    static const uint8_t _pinWhite;
    static unsigned _OffWhite[3];
    int _oldHour, _oldMinute;
    unsigned int _valOnDisplay;
    PanelData* _TheData;
    boolean _whiteOn;
    void _allOn(uint8_t value);
    void _setColorPins();
    void _fadeIn(unsigned digits, unsigned digBefore, int maxByte, uint8_t pins[]);
     
};

#endif

