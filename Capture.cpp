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

#include "Capture.h"


uint16_t CapturedDataStruct::getPeakSampleValue()
{
    if(currentSample < NUM_SAMPLES) return 0;
    uint16_t startValue = (triggerLocation <0)? 0: triggerLocation;
    uint16_t maxvalue = 0;
    for(uint16_t x = startValue; x< currentSample; x++)
    {
        if(buffer[x] > maxvalue) maxvalue = buffer[x];
    }
    return maxvalue;
}


//alarm_pool_t * Capture::timerAlarmPool = NULL;
bool Capture::analogInitialized = false;

Capture::Capture(uint16_t adcChannel)
{
    m_adcChannel = adcChannel;
    if(!analogInitialized)
    {
        adc_init();
        adc_gpio_init(26);  // Enable ADC on GPIO 26     
        adc_select_input(m_adcChannel);
    }
    for(uint16_t seg = 0; seg<10; seg++) baselines[seg] = 20;    // Default baseline is 0.05 V

}

Capture::~Capture()
{
    cancel_repeating_timer(&sampling_timer);
}


bool Capture::staticTimerCallback(struct repeating_timer *t)
{
    return(((Capture *)t->user_data)->timerCallback(t));
}


uint16_t Capture::getPeakVoltage()
{
    uint16_t result = peakADCSinceLastRequest;
    peakADCSinceLastRequest = 0;
    return result;
}

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

uint16_t Capture::getFrequency()
{
    return currentFrequency;
}

bool Capture::timerCallback(struct repeating_timer *t)
{
    uint64_t currentTime = time_us_64();
    uint16_t currentADC = adc_read()>>2;    // We'll only use 10 bits
    // Keep track of the peak voltage since the last voltage request
    if(currentADC > peakADCSinceLastRequest) peakADCSinceLastRequest = currentADC;
    if(currentADC < lowADCforPeriod) lowADCforPeriod = currentADC;
    if(searchingForZero)
    { 
        if(currentADC < baseline){    // Below baseline (default 0.05V - allowing for rounding the lowest 10 count/ 0.05V to ground)
            searchingForZero = false;
        }
    }
    else
    {
        if(currentADC > baseline + 30)    // Must be about 0.1V above baseline (20 counts = 0.1V)
        {
            frequencyCyclesCounted++;
            searchingForZero = true;
            // First trigger when capturing
            if(captureInProgress && currentCaptureBuffer->triggerLocation<0) currentCaptureBuffer->triggerLocation = currentCaptureBuffer->currentSample;
        }
    }

    if(currentTime > tenthOfSecondSamplingEndTime)
    {
       // This logic tracks the minimum voltage over the past 10 sampling periods and considers that the baseline for frequency counting. This means it
        // can take up to a second to readjust if the lowest signal voltage rises. But it will respond quickly if the lowest voltage drops
        // We do not adjust the baselines if capturing
        uint16_t minValue = 512;    // Baseline is never above about 2.5V
        for(uint16_t seg = 0; seg<9; seg++)
        {
            if(baselines[seg] < minValue) minValue = baselines[seg];
            baselines[seg] = baselines[seg+1];
        }
        baselines[9] = lowADCforPeriod;
        baseline = minValue + 20; // Baseline is about .1 V over the lowest detected value;
        lowADCforPeriod = 512;
        tenthOfSecondSamplingEndTime = currentTime + 100000;
    }

    // Has freq sampling time passed?
    if(currentTime > frequencySamplingEndTime)
    {
        if(frequencyCyclesCounted == 0 && fullSecondSample)
        {
            currentFrequency = 0;
            fullSecondSample = false;
        }
        else if(frequencyCyclesCounted < 1000 && !fullSecondSample)
        {
            frequencySamplingEndTime = frequencySamplingEndTime + 900000;    // Extend measurement to one second
            fullSecondSample = true;
        }
        else 
        {
            currentFrequency = (fullSecondSample)? frequencyCyclesCounted: frequencyCyclesCounted * 10;
            frequencySamplingEndTime = currentTime + 100000;
            fullSecondSample = false;
            frequencyCyclesCounted = 0;
        }
    }
    if(!captureInProgress || !currentCaptureBuffer) return true; // No capturing or buffer not defined
    currentDividerCount-=1;
    if(currentDividerCount > 0) return true;
    currentDividerCount = currentCaptureBuffer->divisor;
    if(currentCaptureBuffer->currentSample < sizeof(currentCaptureBuffer->buffer))
    {
        currentCaptureBuffer->buffer[currentCaptureBuffer->currentSample++] = currentADC;
        if( (currentCaptureBuffer->triggerLocation>=0 && (currentCaptureBuffer->currentSample - currentCaptureBuffer->triggerLocation) > NUM_SAMPLES+1) ||
            (currentCaptureBuffer->triggerLocation==-1 && currentCaptureBuffer->currentSample >= NUM_SAMPLES) )
        {
            // We have NUM_SAMPLES valid samples past the trigger, we're done
            // Or we've captured NUM_SAMPLES DC (trigger not found)
            captureInProgress = false;
            currentCaptureBuffer->endFrequency = currentFrequency;
            currentCaptureBuffer->captureComplete = true;
            return true;
        }
        return true;    // On to next cycle
    }
    // This shouldn't ever happen, but just in case, let's not overflow the buffer.
    captureInProgress = false;
    currentCaptureBuffer->endFrequency = currentFrequency;
    currentCaptureBuffer->captureComplete = true;
    return true;
}

// Call with NULL parameter to initially start the timer
bool Capture::startCapture(CapturedDataStruct *cds)
{
    if(captureInProgress) return false;
    currentCaptureBuffer = cds;
    if(cds != NULL)
    {
        currentDividerCount = cds->divisor;
        cds->currentSample = 0;
        cds->triggerLocation = -1;
        captureInProgress = true;
    }
    if(!timerOn)
    {
        timerOn = true;
        //memset(&sampling_timer, 0, sizeof(sampling_timer)); // Clear the existing structure
        if (!  add_repeating_timer_us(-SAMPLE_RATE_US, staticTimerCallback, this, &sampling_timer)) {
            printf("Failed to start ADC sampling timer!");
            captureInProgress = false;
            return false;
        }
        frequencySamplingStartTime = time_us_64();
        frequencyCyclesCounted = 0;
        frequencySamplingEndTime = 100000;  // 100ms default sampling time
        tenthOfSecondSamplingEndTime = 100000;  // 100ms 1/10 second sampling time
    }
    
    return true;

}
