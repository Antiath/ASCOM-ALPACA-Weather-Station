# ASCOM-ALPACA-Weather-Station
A WiFi weather station for astronomy purposes based on the ESP32 with ASCOM-ALPACA interface.

## Description

This project aims at developping a weather station that can be used to help astrophotographers monitoring the sky conditions. 

It measures :
- Temperature
- Relative Humidity
- Dew point
- Cloud sensor measuring sky temperature (typicaly clear sky when below -15°C)
- Rain detection
- Sky quality meter measuring the luminosity of the sky background
- Wind speed with a cup anemometer

The station provides multiple ways to monitor and use those measurements.
Local :
- ASCOM/ALPACA interface. This is the primary way to use the station. It provides a server that ASCOM clients can connect to through the ALPACA protocol. It will be available as a device of the ascom type "Observing conditions" (in Nina, this is the Weather tab), after you gave to the client the local IP address of the station. You can then, for example, use a Safity monitor to read those measurements and determine a Safe/Unsafe flag that let your software know if it has to stop a photo session. Personnaly I use the ascom driver Environment Safery monitor for that (https://www.dehilster.info/astronomy/ascom_environment_safetymonitor.php).
- A webpage that can display the last measuements and let you upload new parameters to the station.
Online :
- ThingSpeak. This is an online platform that will make gather data and display nice graphs.

## Hardware

Sensors : 
- Temperature + Humidity sensor : SHT31 in a waterproof housing (from mine is from DF Robots) ( Bus : i2c)
- Cloud sensor : IR sensor MLX90614 from Melexis ( Bus : i2c)
- Rain detection : Optical rain sensor Hydreon RG11 (Bus: a digital input of the ESP32, Pin 26 by default)
- Sky quality meter : TSL237 + Lens ( ex : a 20° LED lens) + IR/UV cut filter ( ESP32 side : i2c. Teensy side: Pin 13) + a waterproof housing.
- Anemometer : a switch based (2 wires) cup anemometer that sends a pulse every rotation. ( ESP32 side : i2c. Teensy side: Pin 3)

Microcontrollers :
- ESP32
- Teensy 3.2 ( yes yo need both but AVR Arduinos can also replace the teensy, with few corrections to the code and wiring)

## Getting Started

### Dependencies

- Arduino
- Teensyduino ( if the teesny is used)
- OS : Windows for now. In the future Linux too when an Indi implementation of Alpaca is available.
- Software : any ASCOM compliant software ( ex: NINA, APT, SGP, Voyager, PRISM) + the last ASCOM platform.

### Installing the firmware

Download the repo
You will need to initialize the EEPROM of the ESP32 in order to save your wifi credentials into it.

1) - Open CSSBV2_ESP32_Alpaca_webpage.ino and uncomment the EEPROM instructions in the setup() function (line 199 to 222).
   - Find the instructions
   ```
   StoreString(40,"ssid"); //wifi ssid
   StoreString(60,"password"); //wifi password
   ```
   and put your SSID and wifi password between the "".
   Upload CSSBV2_ESP32_Alpaca_webpage.ino.
   If you have a thingspeak account, find the following lines
   ```
   StoreString(160,"aaa"); //Thingspeak write api key
   StoreString(180,"aaa"); //Thingspeak channel id
   ```
   and do the same with your API write key and Channel id (a 6 digit number presumably).
   Then upload CSSBV2_ESP32_Alpaca_webpage.ino to the ESP32.
   Then recomment the section as it was before ( you can also just redownload the code and use this one) and upload CSSBV2_ESP32_Alpaca_webpage.ino again.
 
2) Upload DaugtherBoard_firmware.ino to the second microcontroller ( teensy 3.2 by default). It should be connected to the ESP32 via the i2C bus.

If your wiring is correct and your wifi station in range, it should connect, display the ip adress on the serial monitor and start the server. 

### Installing the driver ( Windows only for now)

With Alpaca, a specific driver is not needed anymore. You can just create a generic driver yourself actually and select that one in your Astro software. Here's how.

1) Open the software ASCOM Diagnostics that comes with the ASCOM platform Then click Choose device > Choose and Connect to Device
2) Select Type ObservingConditions and click Choose
3) Click Alpaca > Create Alpaca driver.
4) Enter the name you want to give to the driver and hit OK. Your driver is created and now you just need to provide the IP address of the station by clicking on the Properties button and filling the required field. Port should be 4040 by default (if not, set it to that)

### Executing program

1) Open the IP adress of the weather station in your browser on port 4040, with http (https won't work) like http://192.168.0.125:4040. It should load the webpage.
2) Open your favorite Astronomy software and, in the list of Weather devices, you can now find the driver created above. Connect and it should get measurements from the esp32.

## Hardware build 

### Wiring

Not written yet sorry.

### Building the station

Not written yet sorry.

## Version History

* 0.1
    * Initial Release

## Acknowledgments
A huge thank you goes to these people who provided excellent work, enabling the server side of this project.
* [Alpaca server]: this project reusues a lot of code from R.Brown and especially from it's focuser controller myFP2ESP32 (https://sourceforge.net/projects/myfocuserpro2-esp32/ - full list of authors in the code)
* [Web server]: the webpage is inspired by the work of MoThunderz (https://github.com/mo-thunderz and especially https://github.com/mo-thunderz/Esp32WifiPart2)
