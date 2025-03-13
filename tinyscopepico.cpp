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

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/timer.h"
#include "hardware/watchdog.h"
#include "Scope.h"

extern "C"
{
    #include "ssd1306.h"
    #include "font.h"
}

const uint LED_PIN = 25;
const uint MODE_PIN = 15;

// I2C defines
// This example will use I2C0 on GPIO8 (SDA) and GPIO9 (SCL) running at 400KHz.
// Pins can be changed, see the GPIO function select table in the datasheet for information on GPIO assignments
#define I2C_PORT i2c0
#define I2C_SDA 4
#define I2C_SCL 5

ssd1306_t disp;

int main()
{
    sleep_ms(10);  // Some setup time
    stdio_init_all();
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    // I2C Initialisation. Using it at 400Khz.
    i2c_init(I2C_PORT, 400*1000);
    sleep_ms(10);  // Some setup time
    
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);
    // For more examples of I2C use see https://github.com/raspberrypi/pico-examples/tree/master/i2c

    // Set up the mode switch
    gpio_init(MODE_PIN);
    gpio_set_dir(MODE_PIN, GPIO_IN);
    gpio_pull_up(MODE_PIN);

    // Watchdog example code
    if (watchdog_caused_reboot()) {
        printf("Rebooted by Watchdog!\n");
        // Whatever action you may take if a watchdog caused a reboot
    }
    
    // Enable the watchdog, requiring the watchdog to be updated every 100ms or the chip will reboot
    // second arg is pause on debug which means the watchdog will pause when stepping through code
    watchdog_enable(2000, 1);
    
    Scope activeScope;


 
    disp.external_vcc=false;
    ssd1306_init(&disp, 128, DISPLAYHEIGHT, 0x3C, I2C_PORT);
    ssd1306_poweron(&disp);
    ssd1306_clear(&disp);
    ssd1306_draw_string(&disp, 2, 4, 2, "BitScanner");


    ssd1306_show(&disp);

    uint64_t watchdog_time = time_us_64();
    uint64_t led_time = time_us_64();
    int ledstate = 0;

    bool debouncingMode = false;
    bool stableMode = true;
    bool lastMode = true;
    uint64_t lastModeCheck = time_us_64();



    while (true) {
        activeScope.poll();
        uint64_t current_time = time_us_64();

        /*if(current_time - led_time > 500000)
        {
            ledstate = (ledstate==0)? 1: 0;
            gpio_put(LED_PIN, ledstate);
            led_time = current_time;
            printf("Frequency: %d\n", activeScope.getCurrentFrequency());
            printf("Voltage: %f\n", activeScope.getCurrentVoltage());
        }
        */

        if(current_time - watchdog_time > 1000000)
        {
            // Once/second update the watchdog
            watchdog_update();
            watchdog_time = current_time;
        }

        bool currentMode = gpio_get(MODE_PIN);
        if(!debouncingMode)
        {
            if(currentMode != stableMode) 
            {
                debouncingMode = true;
                lastModeCheck = time_us_64();
                lastMode = currentMode;
            }
        }
        else
        {   // Debouncing
            if(currentMode != lastMode)
            {
                lastMode = currentMode;
                lastModeCheck = time_us_64();
            }
            else if (current_time - lastModeCheck > 50000)
            {
                // It's stable
                debouncingMode = false;
                stableMode = currentMode;
                if(stableMode == 0) activeScope.toggleDisplayMode();    // On button press, toggle the display
            }
        }


    }
}

