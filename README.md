# PICKLE is BETTER than RileyLink


## Warning

This is code for developers only. Here is **very preliminary** code, intended for
collaboration among developers. It **is not ready** for end users
and may be subject to rebasing without notice.

## Hardware

This project has been developed and tested on
a TTGO ESP32 868/915 MHz LoRa OLED module *(version 1 only, see below!),*
which contains an ESP32 SoC, an RFM95 LoRa radio,
a 128x64 pixel SSD1306 OLED display, and a LiPo battery charger.

The module has two push-buttons.
One is hard-wired to reset the board;
the other is available as an input or interrupt source.

Now my idea is to convert code to **very** low energy and minimalistic thing to use with clear ESP32 and module like RFM69


## Hardware Setup

### Antenna

Attach an appropriate antenna to the U.FL connector on the module
before using this software.

### Power

The module can be powered via the micro-USB connector or with a 3.7V
LiPo battery.

The battery connects to a 2-pin female JST 1.25mm connector.

## Software Setup

### Set up the ESP32 environment

1. [Follow these instructions to install the ESP-IDF development environment.](https://docs.espressif.com/projects/esp-idf/en/latest/get-started/index.html#installation-step-by-step)

1. Build and flash one of the example applications to make sure you have a working setup.

## Building

1. Type `make` in the top level of this repository

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

Some of the applications require the pump serial number or frequency
to be defined in the `include/pump.h` file.
It should look like this:

 #define PUMP_ID		"123456"	// pump serial number (note that this is a string constant)
 #define PUMP_FREQUENCY	868500000	// pump frequency
 #define MMTUNE_START	868300000	// starting frequency for mmtune scans


### Time zone configuration

The local time zone is hard-coded in the `include/timezone.h` file.
For example (Moscow):

    #define TZ	"MSK-3MSD

The time zone must be in one of the first two formats
[specified here.](https://www.gnu.org/software/libc/manual/html_node/TZ-Variable.html)
In particular, the "America/New_York" format is *not* supported.
