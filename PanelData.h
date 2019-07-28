///
/// @file	PanelData.h 
/// @brief	Header
/// @details	<#details#>
/// @n	
/// @n @b	Project LEDPanel1
/// @n @a	Developed with [embedXcode](http://embedXcode.weebly.com)
/// 
/// @author	Matthias Hölling
/// @author	Matthias Hölling
/// @date	26.11.12 21:17
/// @version	<#version#>
/// 
/// @copyright	© Matthias Hölling, 2012
/// @copyright	CC = BY NC SA
///
/// @see	ReadMe.txt for references
///



#ifndef PanelData_h
#define PanelData_h

#define SPRING 105,100
#define SUMMER 225,100
#define FALL 45,100
#define WINTER 0,100
#define RGBWHITE 255,195,56
#define MAXXMLSIZE 128

#include "Color.h"


class PanelData {
    
public:
    Color LowerLimit;
    Color UpperLimit;
    Color SeasonColors[4];
    uint8_t backLightStateIndex;
    uint8_t clockLightStateIndex;
    uint8_t moodLightStateIndex;
    uint8_t minuteDisplayed;
    uint8_t hourDisplayed;
    boolean doWhiteAdjust;
    boolean whiteChanged;
    boolean scanner;
    boolean moodColorUpdate;

    PanelData();
    void init();
    void switchOn(boolean switcher);
    boolean whiteNeedsChange() const;
    boolean moodNeedsUpdate() const;
    void clearMoodUpdate ();
    void setWhiteIntensity(unsigned intensity);
    void setMoodIntensity(unsigned intensity);
    unsigned getMoodIntensity() const;
    Color getPresentColor() const;
    void setColorLimits(int colorIndex);
    void fadePresentColorInLimits();
    void setPresentColorInLimits();
    void setPresentColorManual(unsigned hue, unsigned sat);
    void setPresentHue(unsigned hue);
    void setPresentSat(unsigned sat);
    boolean colorNeedsUpdate();
    unsigned int getOffWhite(int RGBIndex);
    void setOffWhite(unsigned value, int index);
    void setBackLight(char*);
    void setMoodLight(char*);
    boolean getWhiteOn();
    boolean moodLightOn(boolean falseOverride);
    void setClockLight(char*);
    char* getXMLData(int index);
    void saveEEPROM() const;
    void loadEEPROM();
    

private:
    Color _PresentColor;
    static const char* _backLightState[4];
    static const char* _clockLightState[3];
    // static const char* _moodLightState[3];
    static const unsigned _initialOffWhite[3];
    boolean _whiteNotUpdated;
    boolean _colorNotUpdated;
    boolean _moodNotUpdated;
    boolean _whiteOn;
    unsigned int _whiteIntensity;
    unsigned int _moodIntensity;
    unsigned int _RGBWhite[3];
    
friend class PanelLight;
    
};


#endif
