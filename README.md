# Bitscanner - A LittleBits/Arduino Compatible Scope/Frequency Counter and Voltmeter - Software

Refer to https://github.com/Desaware/bitscanner/wiki for documentation.

## Software Requirements

Bitscanner is built on the Raspberry Pi Pico 1 (RP2040), using the Pico SDK under Visual Studio Code.

It also uses the pico-ssd1306 library found at https://github.com/daschr/pico-ssd1306, portions of which are included in this repository.

To build, copy the files in this directory to the root of a Raspberry Pi Pico project in VS Code. Be sure to include the CMakeList.txt file. You should then be able to compile and upload the project to your pico device when it is in bootloader mode.

Refer to your Pico documentation or friendly neighborhood AI for instructions on setting up VS Code with the Pico SDK and building and deploying software in that environment.
