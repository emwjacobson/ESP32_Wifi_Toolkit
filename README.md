# ESP32 Wifi Toolkit

This project is still a **HEAVY** work in progress. Many things are destined to change without notice or reason.

## Purpose

This project has a hardware component that I designed, and it's available in the [ESPStick](https://github.com/emwjacobson/ESPStick) repo! Some features of this project do depend on this hardware (such as the button actions), but should have options to disable so it can be used on any generic ESP32 board.

I decided to make a "Wifi Toolkit" project, as I was always interested in how powerful a small little ESP32 board can be.

Some of the features I currently have implemented are:
- Web based UI for control
- Deauther
- SSID Spammer

## Usage

Once the code has been uploaded to the ESP32 board, when powered it should create a wifi accesspoint based on the settings you set. When connected, you can navigate to [http://192.168.4.1/](http://192.168.4.1/) to access the Web UI. This is where the main control done.

## TODO:
- IP Scanner
  - Need to figure out a way to log which IPs are up and down
- Web Server
  - Integrate IP Scanner
  - Button functions
  - System Stats
    - Heap/Memory Usage?
    - Battery Voltage?
- Options disable ESPStick specific hardware
- Implement saving data to SD Card
- Packet capture
  - Need to figure out a way to monitor all channels
    - Disable AP during scan?
  - Integrate with SD Card to save packet file
- Make README look nicer
- Setup menuconfig for components
- Convert IP Scanner to a Task rather than Timer