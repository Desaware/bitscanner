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

#include "Scope.h"
#include <cmath>
extern "C"
{
    #include "ssd1306.h"
    #include "font.h"
}
extern ssd1306_t disp;  // Reference to the diplay (see tineyscopepico.cpp)

Scope::Scope()
{
}

Scope::~Scope()
{
    delete adcCapture;
    adcCapture = NULL;
}

uint16_t Scope::getCurrentFrequency()
{
    return adcCapture->getFrequency();
}

float Scope::getCurrentVoltage()
{
    return getVoltageFromADCValue(adcCapture->getPeakVoltage());
}

float Scope::getVoltageFromADCValue(uint16_t adcvalue)
{
    // We subract 10 to round the lowest 0.05 V to ground
    // We increase the range by 0.1V to account for rounding the lowest to ground and highest to 5V
    float actualVoltage = (adcvalue - 10) * topOfRange / (1023-20);
    if(actualVoltage<0) actualVoltage = 0;
    if(actualVoltage>topOfRange) actualVoltage = topOfRange;
    return std::round(actualVoltage * 10) / 10;
}


bool Scope::startCaptureBasedOnFrequency()
{
    if(adcCapture->captureInProgress) return false; // Already capturing
    uint16_t currentFrequency = getCurrentFrequency();
    uint16_t divider;
    if(currentFrequency <=1000) 
    {
        if(currentFrequency <= 100)
        {
            if(currentFrequency <=10)
            {
                divider = 400;
            }
            else
            {
                divider = 40;
            }
        }
        else
        {
            divider = 4;
        }
    }
    else
    {
        divider = 1;
    }
    caps[currentSampleBuffer].divisor = divider;
    adcCapture->startCapture(&caps[currentSampleBuffer]);
    return true;
}



void Scope::displayVoltage()
{
    char buffer[16];
    ssd1306_clear(&disp);
    sprintf(buffer, "%.1f volts", getCurrentVoltage());
    ssd1306_draw_string(&disp, 2, (DISPLAYHEIGHT - 16)/2, 2, buffer);
    ssd1306_show(&disp);
}
void Scope::displayFrequency()
{
    char buffer[16];
    uint16_t currentFrequency = getCurrentFrequency();
    if(currentFrequency < 1000)
    {
        sprintf(buffer, "%d Hz", currentFrequency);
    }
    else
    {
        float ffreq = currentFrequency;
        sprintf(buffer, "%.2f KHz", ffreq/1000);
    }
    ssd1306_clear(&disp);
    ssd1306_draw_string(&disp, 2, (DISPLAYHEIGHT - 16)/2, 2, buffer);
    ssd1306_show(&disp);
}


void Scope::displayScope()
{
    if(!caps[currentDisplayBuffer].captureComplete) return;    // No data yet
    uint16_t *dataptr = caps[currentDisplayBuffer].buffer;

    ssd1306_clear(&disp);

    int16_t startpos = caps[currentDisplayBuffer].triggerLocation;

    if(startpos<0)
    {
        // Is it DC?
        uint16_t *chkptr = caps[currentDisplayBuffer].buffer;
        uint16_t minValue = 1024;
        uint16_t maxValue = 0;
        for(int16_t xpos = 0; xpos< 100; xpos++, chkptr++)
        {
            if(*chkptr < minValue) minValue = *chkptr;
            if(*chkptr > maxValue) maxValue = *chkptr;
        }
        int16_t dif = capturedDataToYpos(minValue) - capturedDataToYpos(maxValue);  // Remember, axis is inverted, so Y for minValue is larger than Y for maxValue
        if(dif > 1) startpos =0;   // If less than 1 pixel diff, consider it DC
    }

    if(startpos== -1)
    {
        // DC voltage
        uint16_t dcv = capturedDataToYpos(*dataptr);
        ssd1306_draw_line(&disp, 0, dcv, 99, dcv);
        ssd1306_draw_string(&disp, 102, (DISPLAYHEIGHT - 8)/2, 1, "DC" );
    }
    else
    {
        uint16_t endpos = startpos + 100;
        int16_t previousY = -1;
        dataptr += startpos;
        for(int16_t xpos = 0; xpos< 100; xpos++)
        {
            uint16_t newY = capturedDataToYpos(*dataptr++);
            if(previousY==-1 || previousY==newY)
            {
                //printf("%d ", newY);
                ssd1306_draw_pixel(&disp, xpos, newY);
            }
            else 
            {
                //printf("%d - %d ",  previousY, newY);
                if(previousY < newY) ssd1306_draw_line(&disp, xpos, previousY, xpos, newY);
                else ssd1306_draw_line(&disp, xpos, newY, xpos, previousY);
            }
            previousY = newY;
        }
        const char *timescale1;
        const char *timescale2;
        switch(caps[currentDisplayBuffer].divisor)
        {
            case 1: timescale1 = "2.5"; timescale2 = "ms";
            break;
            case 4: timescale1 = "10"; timescale2 = "ms";
            break;
            case 40: timescale1 = "100"; timescale2 = "ms";
            break;
            case 400: timescale1 = "1"; timescale2 = "sec";
            break;
        }
        ssd1306_draw_string(&disp, 102, (DISPLAYHEIGHT - 16)/2, 1, timescale1 );
        ssd1306_draw_string(&disp, 102, (DISPLAYHEIGHT - 16)/2 + 8, 1, timescale2 );

    }

    ssd1306_show(&disp);
}

void Scope::toggleDisplayMode()
{
    switch(currentDisplayMode)
    {
        case ScopeDisplayMode::scope:
            currentDisplayMode = ScopeDisplayMode::voltage;
        break;
        case ScopeDisplayMode::voltage:
            currentDisplayMode = ScopeDisplayMode::frequency;
        break;
        case ScopeDisplayMode::frequency:
            currentDisplayMode = ScopeDisplayMode::scope;
        break;
    }
    updateDisplay();
}

void Scope::updateDisplay()
{
    printf("Updating display\n");
    lastDisplayUpdate = time_us_64();
    switch (currentDisplayMode)
    {
        case ScopeDisplayMode::scope:
            displayScope();
        break;
        case ScopeDisplayMode::voltage:
            displayVoltage();
        break;
        case ScopeDisplayMode::frequency:
            displayFrequency();
        break;
    }
}

void Scope::poll()
{
    uint64_t currentPollTime = time_us_64();    // Mark time of this loop

    if(!adcCapture) adcCapture = new Capture(0);
    if(!adcCapture)  return;
    if(!adcCapture->getTimerOn()) adcCapture->startCapture(NULL);
    // Don't do anything else for 1.5 seconds after power up to allow time for initial frequency count to take place
    if(currentPollTime < 1500000LL) return;

    if( !adcCapture->captureInProgress &&( currentDisplayBuffer == -1 || !caps[currentDisplayBuffer].captureComplete))
    {
        // We've completed a capture!  Or, we're doing the first capture
        currentDisplayBuffer = currentSampleBuffer;
        currentSampleBuffer = (currentSampleBuffer==0)? 1: 0;   // Flip the sample buffer
        startCaptureBasedOnFrequency();     // Start the next capture
    }

    if(currentDisplayMode == ScopeDisplayMode::scope  && (currentPollTime - lastDisplayUpdate) > SCOPEUPDATEUS && caps[currentDisplayBuffer].captureComplete)
    {
        updateDisplay();
        caps[currentDisplayBuffer].captureComplete = false;
    }

    // Update voltage or frequency every 500ms
    if( (currentDisplayMode == ScopeDisplayMode::voltage || currentDisplayMode == ScopeDisplayMode::frequency) && (currentPollTime - lastDisplayUpdate) > VORFUPDATEUS)
    {
        updateDisplay();
    }

}

