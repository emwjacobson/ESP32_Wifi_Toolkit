# ESP32 Wifi Toolkit

This project is still a **HEAVY** work in progress. Many things are destined to change without notice or reason.

## Purpose

This project has a hardware component that I designed, and it's available in the [ESPStick Hardware](https://github.com/emwjacobson/ESPStick_Hardware) repo! Some features of this project do depend on this hardware (such as the button actions), but should have options to disable so it can be used on any generic ESP32 board.

I decided to make a "Wifi Toolkit" project, as I was always interested in how powerful a small little ESP32 board can be.

Some of the features I currently have implemented are:
- Web based UI for control
- Deauther
- SSID Spammer

## Usage

Once the code has been uploaded to the ESP32 board, when powered it should create a wifi accesspoint based on the settings you set. When connected, you can navigate to [http://192.168.4.1/](http://192.168.4.1/) to access the Web UI. This is where the main control done.

## TODO:
- Web Server
  - SSID Spammer
  - Change SSID
  - Local IP Scanner
  - Button functions
- Options disable ESPStick specific hardware
- Handshake capture
- Make README look nicer
- Setup menuconfig for some components