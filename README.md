# PICKLE is BETTER than RileyLink


## Hardware

This project has been developed and tested on
a TTGO ESP32 868/915 MHz LoRa OLED module *(version 1 only, see below!),*
which contains an ESP32 SoC, an RFM95 LoRa radio, and a LiPo battery charger.

The module has two push-buttons.
One is hard-wired to reset the board;
the other is available as an input or interrupt source.

Now my idea is to convert code to **very** low energy and minimalistic thing to use with clear ESP32 and module like RFM69


## Hardware Setup

### Antenna

Attach an appropriate antenna to the U.FL connector on the module
before using this software.

I highly recommend buying 2-3 other antennas for testing. Some of the modules have very bad antenna configurations. Feel free to test another. The best solution for me is the piece of wire length of 75 millimeters (-66 dBi (+-2 dBi) at 5 meters).

### Power

The module can be powered via the micro-USB connector or with a 3.7V
LiPo battery.

The battery connects to a 2-pin female JST 1.25mm connector.

WARNING!

The module has not contained battery protection from discharging. It 100% drain your battery - which can be dangerous. Please use a protection board like TP4056 (with 4 pins output) or be careful.

### Display

In my version, I have removed display (you can also buy hardware without a display at Aliexpress) and removed any display-specific commands from the code.


## Software Setup

### Set up the ESP32 environment

1. [Follow these instructions to install the ESP-IDF development environment.](https://docs.espressif.com/projects/esp-idf/en/latest/get-started/index.html#installation-step-by-step)

1. Build and flash one of the example applications to make sure you have a working setup.

## Building

1. Type `make` at the top level of this repository

1. Change to the `project` subdirectory

1. Build the project by running `make -j`

1. Flash the project to your ESP32 module by running `make flash`

## Running

Run [Loop](https://loopkit.github.io/loopdocs/) on your iPhone.
Pickle should show up when you scan for a RileyLink.

Pickle will show messages on the OLED display when your phone
connects and disconnects. Pushing the button will display the current
status. Pickle may not respond to the button press immediately if it is
communicating with the pump, due to scheduling priorities.

### Pump-specific configuration

If you using American pump, you should set a frequency in the `src/gnarl/main.c` file.
It should look like this:

 #define PUMP_FREQUENCY 916600000 // pump frequency

 If you using WW pump you have nothing to change.


