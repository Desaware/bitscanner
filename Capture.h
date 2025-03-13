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

#ifndef __CAPTURE_H__
#define __CAPTURE_H__

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/timer.h"
#include "hardware/adc.h"

#define NUM_SAMPLES 100
#define SAMPLE_RATE_US 25

typedef struct CapturedDataStruct
{
    uint16_t divisor = 1;   // # of SAMPLE_RATE_US events for each data point captured
    uint16_t buffer [ NUM_SAMPLES * 2 ] = {};    // The sample buffer
    int16_t triggerLocation = -1;   // Location of triggered data (0 - NUM_SAMPLES-1)
    uint16_t currentSample = 0;     // Location for next sampled data
    uint16_t endFrequency = 0;      // Frequency captured at the end of the frame
    bool captureComplete = false;   // Set to true when capture is complete
    uint16_t getPeakSampleValue();
} CapturedData;

class Capture
{
    public:
        Capture(uint16_t adcChannel);
        ~Capture();

        uint16_t *samples;

        bool captureInProgress = false;

        bool timerCallback(struct repeating_timer *t);

        uint16_t getPeakVoltage();

        uint16_t getFrequency();

        bool getTimerOn() { return timerOn; }

        static bool staticTimerCallback(struct repeating_timer *t);

        // Returns true on success, false on failure. buffer must be large enough to hold # of samples from constructor
        // The divider specifies to skip that number of entries to capture lower frequencies
        // For example: With the default 25us clock, it's 40Khz, 4 = 10khz, 40 = 1khz, 400 = 100hz, 4000 = 10hz
        bool startCapture(CapturedDataStruct *cds);

        //static alarm_pool_t * timerAlarmPool;

    private:
        static bool analogInitialized;

        CapturedDataStruct *currentCaptureBuffer;

        uint16_t m_adcChannel = 0;
        uint16_t currentDividerCount;

        uint64_t frequencySamplingStartTime = 0;
        uint64_t frequencySamplingEndTime = 0;
        uint64_t tenthOfSecondSamplingEndTime = 0;
        uint32_t frequencyCyclesCounted = 0;
        uint16_t peakADCSinceLastRequest = 0;
        bool    fullSecondSample = false;   // fullSecondSample is true if sampling frequency over an entire second

        uint32_t currentFrequency = 0;  // 10X the frequency (to allow one decimal digit without floating point math)
        bool searchingForZero = false;  // Looking for zero. True means we are at zero.

        uint16_t baselines[10];     // Lowest ADC value in the last 10 1/10 second periods.
        uint16_t baseline = 20;     // Current baseline value for frequency counting
        uint16_t lowADCforPeriod = 512;

        bool timerOn = false;
        struct repeating_timer sampling_timer = {};  

};















#endif