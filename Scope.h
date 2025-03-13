/*
    Bitscanner - a LittleBits compatible combination oscilloscope, voltmeter and frequency counter
    Copyright (C) 2025 by Dan Appleman

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/    

#ifndef __SCOPE_H__
#define __SCOPE_H__

#include "Capture.h"


// Adjust R1 and R2 to measured values if you wish to calibrate
#define R1  15.0  // R1 used in voltage divider
#define R2  27.0  // R2 used in voltage divider

#define DISPLAYHEIGHT 32
#define SCOPEUPDATEUS   500000LL
#define VORFUPDATEUS    500000LL

enum ScopeDisplayMode
{
    scope,
    voltage,
    frequency
};


class Scope
{
    public:
        Scope();
        ~Scope();

        void poll();

        uint16_t getCurrentFrequency();

        float getCurrentVoltage();

        ScopeDisplayMode getDisplayMode() { return currentDisplayMode; }
        void toggleDisplayMode();               // Toggle display mode among the options (switch hit)


    private:
        int16_t currentSampleBuffer = 0;       // 0 or 1 indicating which CaptureData structure is currently being sampled
        int16_t currentDisplayBuffer = -1;     // -1 on power up, otherwise which CaptureData structure is being displayed

        CapturedData caps[2];

        ScopeDisplayMode currentDisplayMode = scope;    // Display mode shown on the current screen ( to support fast switching )

        Capture *adcCapture = NULL;
        float topOfRange = 3.3 * (R1 + R2) / R2;    // Measured top of range based on voltage divider

        float getVoltageFromADCValue(uint16_t adcvalue);

        // Scale to display height. Scale value is averaged when divided
        inline uint16_t capturedDataToYpos(uint16_t capturedData)
        {
            uint16_t result =  ((DISPLAYHEIGHT == 32)?   (capturedData+16)>>5 : (capturedData+8) >> 4) ;
            if(result > (DISPLAYHEIGHT-1)) result = DISPLAYHEIGHT - 1;
            return (DISPLAYHEIGHT-1) - result;  // And invert
        }

        bool startCaptureBasedOnFrequency();
        void updateDisplay();
        void displayVoltage();
        void displayFrequency();
        void displayScope();

        uint64_t lastDisplayUpdate = 0;

};

#endif
