/*----------------------------------------------------------------------
Clear Sky Sensor Box V2 - ESP 32 Main board Firmware
----------------------------------------------------------------------

!!!!!! README FIRST!!!!!!!!
For a first upload, you need to initialise the EEPROM. Go in the setup() fucntion to uncomment the relevant part, upload, and then comment again and  reupload.

----------------------------------------------------------------------
Description
----------------------------------------------------------------------

Code for an ASCOM-Alpaca Weather Station on the ESP32.

It is heavily based on the firmware of Robert Brown and Holger M. for their ESP32 focuser MyFP2ESP32.
Skimmed down and adapted for the ASCOM device type "Observing Conditions".
Refer to https://sourceforge.net/projects/myfocuserpro2-esp32/ for the original code of MyFP2ESP32


Remark: Not every properties and methods of the ASCOM specifications are implemented in this firmware (I don't use a pressure sensor for example) but it should be easy enough to adapt to your specific needs.

Written by F.Mispelaer a.k.a Antiath (last update: 3-01-2024)
Version : 0.1
---------------------------------------------------------------------------------------

----------------------------------------------------------------------
MIT License
----------------------------------------------------------------------

Copyright (c) 2023 Antiath

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.


*/



//----------------------------------------------------------------------
//INCLUDES
//----------------------------------------------------------------------

//To Download from the library manager
#include <WiFi.h>  // needed to connect to WiFi
#include <WiFiUdp.h>
#include <ArduinoJson.h>  // needed for JSON encapsulation (send multiple variables with one string)
#include <WebServer.h>    
#include <WebSocketsServer.h>  // needed for instant communication between client and server through Websockets
#include <DFRobot_SHT3x.h>
#include <elapsedMillis.h>
#include <Adafruit_MLX90614.h>
#include <ThingSpeak.h>
#include <EEPROM.h>
//Custom library
#include "Definitions.h"


//----------------------------------------------------------------------
//Wiring
//----------------------------------------------------------------------

//Hydreon rain sensor connector 1:+12V ; 2:GND ; 3:SIG ; //Default to pin 26, edit RAINPIN in Definition.h

//----------------------------------------------------------------------
//EEPROM map
//adress : variable
//----------------------------------------------------------------------
//7 : Samplingperiod in seconds
//8 : Averageperiod in seconds
//9 : SQM calibration factor darkfreq
//20 : SQM calibration factor A
//31 : Wind calibration factor to divide by 10
//32 : Bool Cloud internal Temp(1) or SHT31(0)
//33 : K1
//34 : K2
//35 : K3
//36 : K4
//37 : K5
//38 : K6
//39 : K7
//40 : Wifi SSID1
//60 : Wifi pwd1
//80 : Wifi SSID2
//100 : Wifi pwd2
//120 : Acces Point SSID
//140 : Acces Point pwd
//160 : ThingSpeak Write API key
//180 : ThingSpeak Id


#define EEPROM_SIZE 201

//General CLASS INSTANCES----------------------------

DFRobot_SHT3x sht3x(&Wire, /*address=*/0x44, /*RST=*/4);
Adafruit_MLX90614 mlx = Adafruit_MLX90614();
WiFiClient ThingSpeakClient;
elapsedMillis Windmillis, samplingTimer, meanTimer;

//Ascom_server ascomsrv();
//==============================


//GLOBALS------------------------------------

//Mean Observing Conditions
int averageperiod = 60;        //s
int samplingperiod = 2;        //s
float dewpoint = 4.0;          //°C
int humidity = 50;             //%
int rainrate = 0;              //0 no rain, 1 rain
float skyquality = 18.0;       //mag/arcsec²
float skytemperature = -15.0;  //°C
float temperature = 10.0;      //°C
float windspeed = 27.0;        //km/h


//Mean buffers
float TMPval = 0;
float TMPprev = 20;
long HRval = 0;
long Rainval = 0;
float Tskycorrval = 0;
float windval = 0;
float sqmval = 0;
//==============================

//Measure variables
bool flagCloud;
int K1, K2, K3, K4, K5, K6, K7;
float mySQMreading = 0.0;  // the SQM value
const float area = 0.0092;  // surface area of sensor. Not used in the code but it is used  when calibrating A so I leave it as a note.
float A = 20.278;          //calculus for FOV = 20°. //Unihedron A=17.6. You need to calibrate this value. Without a real Unihedron unit, the "easiest" is to use a telescope and a camera to measure the level of sky background, perform photometry with a reference star  and then translate that to mag/arcsec².
float darkFreq = 0.032;
float windcal = 0;
//=============================

// General variables

int MeanCount = 0;
bool flag_sht31=false;


const char* JSONPAGETYPE = "application/json";

// SSID and password of Wifi connection:
String ssid1, pwd1, ssid2, pwd2, AP_ssid, AP_pwd, myWriteAPIKey;
unsigned long myChannelNumber;

char _packetBuffer[255] = { 0 };
unsigned int _ASCOMClientID = 0;
unsigned int _ASCOMClientTransactionID = 0;
unsigned int _ASCOMServerTransactionID = 0;
int _ASCOMErrorNumber = 0;
String _ASCOMErrorMessage = "";
byte _ASCOMConnectedState = 0;

String sensordescription = "default";
double timesincelastupdate = 0;
int interval = 1000;               // send data to the client every 1000ms -> 1s
unsigned long previousMillis = 0;  // we use the "millis()" command for time reference and this will output an unsigned long
//=============================


// NETWORK CLASSES 

WebServer* _ascomserver;
WebSocketsServer webSocket = WebSocketsServer(81);  // the websocket uses port 81 (standard port for websockets
WiFiUDP _ASCOMDISCOVERYUdp;
//=============================


//====================================SETUP FUNCTION====================================

void setup() {
  Serial.begin(115200);  // init serial port for debugging
  delay(3000);
  EEPROM.begin(EEPROM_SIZE);
delay(1000);

/*
//EEPROM INITIALISATION ----- Uncomment those lines if you wish to do a first initialisation of the EEPROM with values (to do once only at the first upload. After that, you will be able to update them in the webpage) 

 EEPROM.put(7,2);
 EEPROM.put(8,60);
 StoreString(9,"0.032");
 StoreString(20,"20.278");
 EEPROM.put(31,24);
 EEPROM.put(32,1);
 EEPROM.put(33,33);
 EEPROM.put(34,0);
 EEPROM.put(35,4);
 EEPROM.put(36,100);
 EEPROM.put(37,100);
 EEPROM.put(38,0);
 EEPROM.put(39,0);
 StoreString(40,"ssid"); //wifi ssid
 StoreString(60,"password"); //wifi password
 StoreString(160,"aaa"); //Thingspeak write api key
 StoreString(180,"aaa"); //Thingspeak channel id
 EEPROM.commit();
*/

  samplingperiod = readStoredbyte(7);
  averageperiod = readStoredbyte(8);

  A = readStoredString(20).toFloat();
  darkFreq = readStoredString(9).toFloat();
  K1 = readStoredbyte(33);
  K2 = readStoredbyte(34);
  K3 = readStoredbyte(35);
  K4 = readStoredbyte(36);
  K5 = readStoredbyte(37);
  K6 = readStoredbyte(38);
  K7 = readStoredbyte(39);
  windcal = readStoredbyte(31) / 10.0;
  flagCloud = readStoredbyte(32);

  ssid1 = readStoredString(40);
  pwd1 = readStoredString(60);
  ssid2 = readStoredString(80);
  pwd2 = readStoredString(100);
  AP_ssid = readStoredString(120);
  AP_pwd = readStoredString(140);
  myWriteAPIKey = readStoredString(160);
  myChannelNumber = readStoredString(180).toInt();


  pinMode(RAINPIN, INPUT);


  _ascomserver = new WebServer(4040);
  
  //Initializing WIFI-------------
  WiFi.begin(ssid1, pwd1);                                                // start WiFi interface
  Serial.println("Establishing connection to WiFi with SSID: " + ssid1);  // print SSID to the serial interface for debugging
  Serial.println(WiFi.macAddress());
  
  while (WiFi.status() != WL_CONNECTED) {  // wait until WiFi is connected
    delay(1000);
    Serial.print(".");
  }
  
  Serial.print("Connected to network with IP address: ");
  Serial.println(WiFi.localIP());  // show IP address that the ESP32 has received from router
  //=============================

  //Initializing i2c bus with sht31 sensor-------------
  
  //If this fails, the flag will prevent the code from poling the sensor. In this case, temperature and humidity won't be updated.
  if (sht3x.begin() != 0) {
    Serial.println("Failed to Initialize the chip, please confirm the wire connection");
    flag_sht31=false;
    delay(1000);
  }
  else{flag_sht31=true;}
  /**
       * readSerialNumber Read the serial number of the chip.
       * @return Return 32-digit serial number.
       */
  Serial.print("Chip serial number");
  Serial.println(sht3x.readSerialNumber());

  /**
       * softReset Send command resets via I2C, enter the chip's default mode single-measure mode,
       * turn off the heater, and clear the alert of the ALERT pin.
       * @return Read the register status to determine whether the command was executed successfully,
       * and return true indicates success.
       */
  if (!sht3x.softReset()) {
    Serial.println("Failed to Initialize the chip....");
    flag_sht31=false;
  }
//=============================

//Setting up the Alpaca discovery server
//Setting up all the callback methods of the normal Alpaca server. This is where we implement the REST based Alpaca API (https://ascom-standards.org/api/)


  _ASCOMDISCOVERYUdp.begin(ASCOMDISCOVERYPORT);

  _ascomserver->on("/", get_setup);
  _ascomserver->on("/setup", get_setup);
  // // handle Management requests
  _ascomserver->on("/management/apiversions", get_man_version);
  _ascomserver->on("/management/v1/description", get_man_description);
  _ascomserver->on("/management/v1/configureddevices", get_man_configureddevices);
  // // handle ASCOM driver client requests
  // _ascomserver->on("/setup/v1/observingconditions/0/setup", HTTP_GET, ascomget_observingconditionssetup);
  // _ascomserver->on("/setup/v1/observingconditions/0/setup", HTTP_POST, ascomget_observingconditionssetup);
  _ascomserver->on("/api/v1/observingconditions/0/connected", HTTP_PUT, set_connected);
  _ascomserver->on("/api/v1/observingconditions/0/interfaceversion", HTTP_GET, get_interfaceversion);
  _ascomserver->on("/api/v1/observingconditions/0/name", HTTP_GET, get_name);
  _ascomserver->on("/api/v1/observingconditions/0/description", HTTP_GET, get_description);
  _ascomserver->on("/api/v1/observingconditions/0/driverinfo", HTTP_GET, get_driverinfo);
  _ascomserver->on("/api/v1/observingconditions/0/driverversion", HTTP_GET, get_driverversion);
  _ascomserver->on("/api/v1/observingconditions/0/averageperiod", HTTP_GET, get_averageperiod);
  _ascomserver->on("/api/v1/observingconditions/0/averageperiod", HTTP_PUT, set_averageperiod);
  //_ascomserver->on("/api/v1/observingconditions/0/cloudcover", HTTP_GET, get_cloudcover);
  _ascomserver->on("/api/v1/observingconditions/0/dewpoint", HTTP_GET, get_dewpoint);
  _ascomserver->on("/api/v1/observingconditions/0/humidity", HTTP_GET, get_humidity);
  _ascomserver->on("/api/v1/observingconditions/0/rainrate", HTTP_GET, get_rainrate);
  _ascomserver->on("/api/v1/observingconditions/0/skyquality", HTTP_GET, get_skyquality);
  _ascomserver->on("/api/v1/observingconditions/0/skytemperature", HTTP_GET, get_skytemperature);
  _ascomserver->on("/api/v1/observingconditions/0/temperature", HTTP_GET, get_temperature);
  _ascomserver->on("/api/v1/observingconditions/0/windspeed", HTTP_GET, get_windspeed);
  _ascomserver->on("/api/v1/observingconditions/0/winddirection", HTTP_GET, get_winddirection);
  _ascomserver->on("/api/v1/observingconditions/0/supportedactions", HTTP_GET, get_supportedactions);
  _ascomserver->on("/api/v1/observingconditions/0/sensordescription", HTTP_GET, get_sensordescription);
  _ascomserver->on("/api/v1/observingconditions/0/timesincelastupdate", HTTP_GET, get_timesincelastupdate);
  // // handle url not found 404
  _ascomserver->onNotFound(get_notfound);
  _ascomserver->begin();

  
  webSocket.begin();                  // start websocket
  webSocket.onEvent(webSocketEvent);  // define a callback function -> what does the ESP32 need to do when an event from the websocket is received? -> run function "webSocketEvent()"
  ThingSpeak.begin(ThingSpeakClient); 
//=============================

  
  Windmillis = 0;
  samplingTimer = 0;
  meanTimer = 0;
  samplingTimer = 0;

}

//====================================MAIN LOOP====================================
void loop() {


  Measure();
  Mean();

  _ascomserver->handleClient();

  // check for ALPACA discovery received packets
  checkASCOMALPACADiscovery();

  // Update function for the webSockets
  webSocket.loop();
  
// Update function that sends the last mean measurtements to the webpage.
  UpdateWebpageReadings();
  
//We restart the board every 48 hours to prevent any memory fragmentation.   
if(millis()>2*24*60*60*1000)ESP.restart();

}

//====================================

//====================================SENSOR FUNCTIONS====================================

// ----------------------------------------------------------------------
// Measure()
//Get the last measurement fromm all the sensors.
// ----------------------------------------------------------------------
void Measure() {
  if (samplingTimer >= (samplingperiod * 1000)) {
    //-----------Getting SQM and Wind values
    //Request SQM anbd wind frequencies from separate Teensy 3.2
    byte RxByte;
    char rec[32];
    String recs;
    int i = 0;
    short j, k;
    String windfreqString, sqmfreqString;
    float windfreq;
    int sqmfreq;

    Wire.requestFrom(0x55, 32);
    // Wire.beginTransmission(0x55); // Transmit to device with address 85 (0x55)
    // Wire.write("send");      // Sends Potentiometer Reading (8Bit)
    // Wire.endTransmission(true);       // Stop transmitting
    while (Wire.available()) {  // Read Received Datat From Slave Device
      RxByte = Wire.read();
      rec[i] = char(RxByte);
      if (rec[i] == '+') j = i;
      else if (rec[i] == '#') k = i;
      i++;
    }

    recs = rec;
    windfreqString = recs.substring(0, j);
    sqmfreqString = recs.substring(j + 1, k);

    windfreq = windfreqString.toFloat();
    if (windfreq > 300.0) windfreq = 0.0;
    sqmfreq = sqmfreqString.toInt();

    //  Serial.print(windfreq);
    //  Serial.print(" ");
    //  Serial.println(sqmfreq);

   float freq = sqmfreq - darkFreq;
    if (freq < 0) { freq = darkFreq; }
    mySQMreading = A - 2.512 * log10(freq);



    float T67, Td, Tskycorr;
    float Tsky = mlx.readObjectTempC();
    float Thr=mlx.readAmbientTempC();
    float tcomp=temperature;
	
    if(flagCloud){tcomp=Thr;}


    if (abs((K2 / 10.) - tcomp) < 1) T67 = sign(K6) * sign(tcomp - K2 / 10.) * abs((K2 / 10.) - tcomp);
    else T67 = (K6 / 10.) * sign(tcomp - K2 / 10.) * (log(abs(((K2 / 10.) - tcomp))) / log(10.) + (K7 / 100.));
    Td = ((K1 / 100.) * (tcomp - (K2 / 10.))) + ((K3 / 100.) * pow((exp(K4 / 1000. * tcomp)), (K5 / 100.))) + T67;
    if ((Tsky > -100.0) && (Tsky < 100.0) && (tcomp > -100.0) && (tcomp < 100.0)) {
      
      Tskycorr = Tsky - Td;  // compensation normale
      //Tskycorr=Tsky-Thr; //compensation non calibrée
    }
    Tskycorrval = Tskycorrval + Tskycorr;
    HRval = HRval + sht3x.getHumidityRH();
    TMPval = TMPval + sht3x.getTemperatureC();

    Rainval = Rainval + digitalRead(RAINPIN);
    windval = windval + (windfreq * windcal);
    sqmval = sqmval + mySQMreading;

    MeanCount = MeanCount + 1;
    //Serial.println(windspeed);
    samplingTimer = 0;
  }
}
//================================================
	
// ----------------------------------------------------------------------
// Mean()
//Compute the mean after an "averageperiod"  amount of seconds
// ----------------------------------------------------------------------	
void Mean() {

  if (meanTimer >= (averageperiod * 1000)) {
    MeanCount = MeanCount - 1;
    skytemperature = (float)Tskycorrval / MeanCount;
    temperature = ((float)TMPval) / MeanCount;
    humidity = (float)HRval / MeanCount;
    dewpoint = temperature - ((100.0 - humidity) / 5.0);
    rainrate = (float)Rainval / MeanCount;
    skyquality = (float)sqmval / MeanCount;
    windspeed = (float)windval / MeanCount;

    UpdateThingSpeak();

    MeanCount = 0;
    Tskycorrval = 0;
    TMPval = 0;
    HRval = 0;
    Rainval = 0;
    sqmval = 0;
    windval = 0;

    meanTimer = 0;
  }
}

//================================================




// ----------------------------------------------------------------------
// UpdateThingSpeak()
// Sends the last means to the provided Thingspeak account
// ----------------------------------------------------------------------	
void UpdateThingSpeak() {

  ThingSpeak.setField(1, temperature);
  ThingSpeak.setField(2, humidity);
  ThingSpeak.setField(3, rainrate);
  ThingSpeak.setField(4, skytemperature);
  ThingSpeak.setField(5, skyquality);
  ThingSpeak.setField(6, windspeed);
  ThingSpeak.setField(7, dewpoint);


    int x = ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey.c_str());  //requires cont char* instead of a string -_-
    //uncomment if you want to get temperature in Fahrenheit
    //int x = ThingSpeak.writeField(myChannelNumber, 1, temperatureF, myWriteAPIKey);

    if (x == 200) {
      Serial.print("Channel update successful.");
      Serial.print(" ");
      Serial.print(temperature);
      Serial.print(" ");
      Serial.print(humidity);
      Serial.print(" ");
      Serial.print(rainrate);
      Serial.print(" ");
      Serial.print(skytemperature);
      Serial.print(" ");
      Serial.print(skyquality);
      Serial.print(" ");
      Serial.print(windspeed);
      Serial.print(" ");
      Serial.println(dewpoint);

    } else {
      Serial.println("Problem updating channel. HTTP error code " + String(x));
    }

}

//================================================

//====================================EEPROM FUNCTIONS====================================

void StoreString(byte adress, String str) {
  int len = str.length() + 1;
  char buf[len];
  str.toCharArray(buf, len);
  for (int i = 0; i < len; i++) EEPROM.put(byte(adress + i), buf[i]);
}

String readStoredString(byte adress) {
  char buf[20];
  EEPROM.get(adress, buf);
  String str = String(buf);
  return str;
}

byte readStoredbyte(byte adress) {
  byte buf;
  EEPROM.get(adress, buf);
  return buf;
}

//================================================

float sign(float f) {
  if (f < 0) return -1;
  else if (f == 0) return 0;
  else if (f > 0) return 1;
}

//====================================WEBPAGE FUNCTIONS====================================

void UpdateWebpageReadings() {

  unsigned long now = millis();                            // read out the current "time" ("millis()" gives the time in ms since the Arduino started)
  if ((unsigned long)(now - previousMillis) > interval) {  // check if "interval" ms has passed since last time the clients were updated
// Serial.print(heap_caps_get_free_size(MALLOC_CAP_8BIT));
// Serial.print("blip");
// Serial.println(heap_caps_get_largest_free_block(MALLOC_CAP_8BIT));
    String jsonString = "";                    // create a JSON string for sending data to the client
    StaticJsonDocument<300> doc;               // create a JSON container
    JsonObject object = doc.to<JsonObject>();  // create a JSON Object
    object["answer"] = "SEND_READINGS";
    object["tmp"] = String(temperature) + "°C";  // write data into the JSON object -> I used "rand1" and "rand2" here, but you can use anything else
    object["hum"] = String(humidity) + "%";
    object["dew"] = String(dewpoint) + "°C";
    object["cloud"] = String(skytemperature) + "°C";
    object["rain"] = String(rainrate);
    object["sqm"] = String(skyquality) + "mag/arcsec²";
    object["wind"] = String(windspeed) + "km/h";
    serializeJson(doc, jsonString);  // convert JSON object to string
    //Serial.println(jsonString);                       // print JSON string to console for debug purposes (you can comment this out)
    webSocket.broadcastTXT(jsonString);  // send JSON string to clients

    previousMillis = now;  // reset previousMillis
  }
}

void UpdateWebpageParam() {
  // read out the current "time" ("millis()" gives the time in ms since the Arduino started)
  // check if "interval" ms has passed since last time the clients were updated
  String jsonString = "";                    // create a JSON string for sending data to the client
  StaticJsonDocument<500> doc;               // create a JSON container
  JsonObject object = doc.to<JsonObject>();  // create a JSON Object
  object["answer"] = "SEND_EEPROM";
  object["averaging"] = String(averageperiod);
  object["sampling"] = String(samplingperiod);
  object["k1"] = String(K1);
  object["k2"] = String(K2);
  object["k3"] = String(K3);
  object["k4"] = String(K4);
  object["k5"] = String(K5);
  object["k6"] = String(K6);
  object["k7"] = String(K7);
  object["flag"] = String(flagCloud);
  object["aFactor"] = String(A, 3);
  object["darkFreq"] = String(darkFreq, 3);
  object["windFactor"] = String(windcal);
  object["APIkey"] = myWriteAPIKey;
  object["channelid"] = String(myChannelNumber);
  object["ssid"] = ssid1;
  object["pwd"] = pwd1;
  //object["apssid"] = AP_ssid;
  //object["appwd"] = AP_pwd;
  serializeJson(doc, jsonString);      // convert JSON object to string
  Serial.println(jsonString);          // print JSON string to console for debug purposes (you can comment this out)
  webSocket.broadcastTXT(jsonString);  // send JSON string to clients
}

// ----------------------------------------------------------------------
// webSocketEvent(args)
//This function will receive data from the webpage and respond accordingly
// ----------------------------------------------------------------------

void webSocketEvent(byte num, WStype_t type, uint8_t* payload, size_t length) {  // the parameters of this callback function are always the same -> num: id of the client who send the event, type: type of message, payload: actual data sent and length: length of payload
    
  switch (type) {                                                                // switch on the type of information sent
    case WStype_DISCONNECTED:                                                    // if a client is disconnected, then type == WStype_DISCONNECTED
      Serial.println("Client " + String(num) + " disconnected");
      break;
    case WStype_CONNECTED:  // if a client is connected, then type == WStype_CONNECTED
      Serial.println("Client " + String(num) + " connected");
      // optionally you can add code here what to do when connected
      break;
    case WStype_TEXT:  // if a client has sent data, then type == WStype_TEXT
      // try to decipher the JSON string received
      StaticJsonDocument<500> doc;  // create a JSON container
      DeserializationError error = deserializeJson(doc, payload);
      if (error) {
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.f_str());
        return;
      } else {

        // JSON string was received correctly, so information can be retrieved:
        String data;
        int buf;

        char str[30];
        //serializeJson(doc, Serial);  // convert JSON object to string
        //Serial.println(" ");
        String g_request = doc["request"].as<String>();
        //Serial.println(g_request);
        if (g_request == "SEND_BACK") {//this request message tells the code to write data the the EEPROM.

          data = doc["_samplingperiod"].as<String>();
          if ((data != "") && (data != String(samplingperiod))) EEPROM.put(7, byte(abs(data.toInt())));
          data = doc["_averageperiod"].as<String>();
          if ((data != "") && (data != String(samplingperiod))) EEPROM.put(8, byte(abs(data.toInt())));

          data = doc["_darkFreq"].as<String>();
          if ((data != "") && (data != String(darkFreq, 3))) StoreString(9, String(abs(data.toFloat()),3));
          data = doc["_aFactor"].as<String>();
          if ((data != "") && (data != String(A, 3))) StoreString(20, String(abs(data.toFloat()),3));
          data = doc["_windFactor"].as<String>();
          buf = int(abs(data.toFloat()) * 10);
          if ((data != "") && (data != String(windcal, 2))) EEPROM.put(31, byte(buf));
          data = doc["_flag"].as<String>();
          if ((data != "") && (data != String(flagCloud))) {
            if (data == "0") EEPROM.put(32, 0);
            else if (data == "1") EEPROM.put(32, 1);
          }
          data = doc["_k1"].as<String>();
          if ((data != "") && (data != String(K1))) EEPROM.put(33, byte(abs(data.toInt())));
          data = doc["_k2"].as<String>();
          if ((data != "") && (data != String(K2))) EEPROM.put(34, byte(abs(data.toInt())));
          data = doc["_k3"].as<String>();
          if ((data != "") && (data != String(K3))) EEPROM.put(35, byte(abs(data.toInt())));
          data = doc["_k4"].as<String>();
          if ((data != "") && (data != String(K4))) EEPROM.put(36, byte(abs(data.toInt())));
          data = doc["_k5"].as<String>();
          if ((data != "") && (data != String(K5))) EEPROM.put(37, byte(abs(data.toInt())));
          data = doc["_k6"].as<String>();
          if ((data != "") && (data != String(K6))) EEPROM.put(38, byte(abs(data.toInt())));
          data = doc["_k7"].as<String>();
          if ((data != "") && (data != String(K7))) EEPROM.put(39, byte(abs(data.toInt())));

          data = doc["_ssid"].as<String>();
          if ((data != "") && (data != ssid1)) StoreString(40, data);

          data = doc["_pwd"].as<String>();
          if ((data != "") && (data != pwd1)) StoreString(60, data);

          data = doc["_APIkey"].as<String>();
          if ((data != "") && (data != myWriteAPIKey)) StoreString(160, data);

          data = doc["_channelid"].as<String>();
          if ((data != "") && (data != String(myChannelNumber))) StoreString(180, data);

          EEPROM.commit();
          delay(1000);
          ESP.restart();
		  
        } else if (String(g_request) == "GET_EEPROM") {//this request message tells the code send back back parameters stored in the EEPROM
       //Serial.println("blip"); 
          UpdateWebpageParam();
        }
      }
      break;
  }
}


//====================================ALPACA FUNCTIONS====================================

// ----------------------------------------------------------------------
// checkASCOMALPACADiscovery()
//This function will answer Alpaca discovery requests
// ----------------------------------------------------------------------

void checkASCOMALPACADiscovery() {
  // (c) Daniel VanNoord
  // https://github.com/DanielVanNoord/AlpacaDiscoveryTests/blob/master/Alpaca8266/Alpaca8266.ino
  // if there's data available, read a packet

  int packetSize = _ASCOMDISCOVERYUdp.parsePacket();
  if (packetSize) {
    char ipaddr[16];
    IPAddress remoteIp = _ASCOMDISCOVERYUdp.remoteIP();
    snprintf(ipaddr, sizeof(ipaddr), "%i.%i.%i.%i", remoteIp[0], remoteIp[1], remoteIp[2], remoteIp[3]);
    // read the packet into packetBufffer
    int len = _ASCOMDISCOVERYUdp.read(_packetBuffer, 255);
    Serial.println(_packetBuffer);
    if (len > 0) {
      // Ensure that it is null terminated
      _packetBuffer[len] = 0;
    }
    // No undersized packets allowed
    if (len < 16) {
      return;
    }
    // 0-14 "alpacadiscovery", 15 ASCII Version number of 1
    if (strncmp("alpacadiscovery1", _packetBuffer, 16) != 0) {
      return;
    }

    String strresponse = "{\"alpacaport\":" + String(4040) + "}";
    uint8_t response[36] = { 0 };
    len = strresponse.length();

    // copy to response

    for (int i = 0; i < len; i++) {
      response[i] = (uint8_t)strresponse[i];
    }
    Serial.println(strresponse);
    _ASCOMDISCOVERYUdp.beginPacket(_ASCOMDISCOVERYUdp.remoteIP(), _ASCOMDISCOVERYUdp.remotePort());
    _ASCOMDISCOVERYUdp.write(response, len);
    _ASCOMDISCOVERYUdp.endPacket();
  }
}

// ----------------------------------------------------------------------
// getURLParameters()
//This function will read any data sent from the ASCOM client and parse.
// ----------------------------------------------------------------------

void getURLParameters() {
  String str;
  int len = *(&sensornamelist + 1) - sensornamelist;

  for (int i = 0; i < _ascomserver->args(); i++) {

    if (i >= ASCOMMAXIMUMARGS) {
      break;
    }
    str = _ascomserver->argName(i);
    str.toLowerCase();
    if (str.equals("clientid")) {
      _ASCOMClientID = (unsigned int)_ascomserver->arg(i).toInt();
    }
    if (str.equals("clienttransactionid")) {
      _ASCOMClientTransactionID = (unsigned int)_ascomserver->arg(i).toInt();
    }
    if (str.equals("averageperiod")) {
      String strtmp = _ascomserver->arg(i);
      averageperiod = abs(int(strtmp.toFloat() * 60 * 60));  //Received in hours, converted to s
      EEPROM.put(adr[8], byte(averageperiod));
    }
    if (str.equals("connected")) {
      String strtmp = _ascomserver->arg(i);
      strtmp.toLowerCase();
      if (strtmp.equals("true")) {
        _ASCOMConnectedState = 1;
      } else {
        _ASCOMConnectedState = 0;
      }
    }
    if (str.equals("sensorname")) {
      String strtmp = _ascomserver->arg(i);
      for (int i = 0; i < len; i++) {
        if (strtmp == sensornamelist[i]) {
          timesincelastupdate = timesincelastupdatelist[i];
          sensordescription = sensordescriptionlist[i];
        }
      }
    }
  }
}

// ----------------------------------------------------------------------
// get_man_setup()
// This function uploads the webpage to the browser
// ----------------------------------------------------------------------

void get_setup() {
  _ASCOMServerTransactionID++;

  _ascomserver->client().println("HTTP/1.1 200 OK");
  _ascomserver->client().println("Content-type:text/html");
  _ascomserver->client().println();
  _ascomserver->client().print(webpage);
}

// ----------------------------------------------------------------------
// get_man_version()
// ----------------------------------------------------------------------
void get_man_version() {

  // url /management/apiversions
  // Returns an integer array of supported Alpaca API version numbers.
  // { "Value": [1,2,3,4],"ClientTransactionID": 9876,"ServerTransactionID": 54321}

  String jsonretstr = "";

  _ASCOMServerTransactionID++;
  _ASCOMErrorNumber = 0;
  _ASCOMErrorMessage = "";
  getURLParameters();
  // addclientinfo adds clientid, clienttransactionid, servertransactionid, errornumber, errormessage and terminating }
  jsonretstr = "{\"Value\":[1]," + addclientinfo(jsonretstr);

  // sendreply builds http header, sets content type, and then sends jsonretstr
  sendreply(NORMALWEBPAGE, JSONPAGETYPE, jsonretstr);
}


// ----------------------------------------------------------------------
// get_man_description()
// ----------------------------------------------------------------------
void get_man_description() {

  // url /management/v1/description
  // Returns cross-cutting information that applies to all devices available at this URL:Port.
  // content-type: application/json
  // { "Value": { "ServerName": "Random Alpaca Device", "Manufacturer": "The Briliant Company",
  //   "ManufacturerVersion": "v1.0.0", "Location": "Horsham, UK" },
  //   "ClientTransactionID": 9876, "ServerTransactionID": 54321 }

  String jsonretstr = "";

  _ASCOMServerTransactionID++;
  _ASCOMErrorNumber = 0;
  _ASCOMErrorMessage = "";
  getURLParameters();
  // addclientinfo adds clientid, clienttransactionid, servertransactionid, errornumber, errormessage and terminating }
  jsonretstr = "{\"Value\":" + String(ASCOMMANAGEMENTINFO) + "," + addclientinfo(jsonretstr);

  // sendreply builds http header, sets content type, and then sends jsonretstr
  sendreply(NORMALWEBPAGE, JSONPAGETYPE, jsonretstr);
}

// ----------------------------------------------------------------------
// get_man_configureddevices()
// ----------------------------------------------------------------------
void get_man_configureddevices() {
  // url /management/v1/configureddevices
  // Returns an array of device description objects, providing unique information for each served device, enabling them to be accessed through the Alpaca Device API.
  // content-type: application/json
  // { "Value": [{"DeviceName": "Super observingconditions 1","DeviceType": "observingconditions","DeviceNumber": 0,"UniqueID": "277C652F-2AA9-4E86-A6A6-9230C42876FA"}],"ClientTransactionID": 9876,"ServerTransactionID": 54321}

  String jsonretstr = "";

  _ASCOMServerTransactionID++;
  _ASCOMErrorNumber = 0;
  _ASCOMErrorMessage = "";
  getURLParameters();
  // addclientinfo adds clientid, clienttransactionid, servertransactionid, errornumber, errormessage and terminating }
  jsonretstr = "{\"Value\":[{\"DeviceName\":" + String(ASCOMNAME) + ",\"DeviceType\":\"observingconditions\",\"DeviceNumber\":0,\"UniqueID\":\"" + String(ASCOMGUID) + "\"}]," + addclientinfo(jsonretstr);

  // sendreply builds http header, sets content type, and then sends jsonretstr
  sendreply(NORMALWEBPAGE, JSONPAGETYPE, jsonretstr);
}

// ----------------------------------------------------------------------
// get_interfaceversion()
// ----------------------------------------------------------------------
void get_interfaceversion() {
  // curl -X GET "http://192.168.2.128:4040/api/v1/observingconditions/0/interfaceversion?ClientID=1&ClientTransactionID=1234" -H  "accept: application/json"
  // response {"Value":2,"ClientID":1,"ClientTransactionID":1234,"ServerTransactionID":1,"Errornumber":"0","Errormessage":"ok"}

  String jsonretstr = "";

  _ASCOMServerTransactionID++;
  _ASCOMErrorNumber = 0;
  _ASCOMErrorMessage = "";
  getURLParameters();
  // addclientinfo adds clientid, clienttransactionid, servertransactionid, errornumber, errormessage and terminating }
  //jsonretstr = "{\"Value\":2," + addclientinfo( jsonretstr );
  //jsonretstr = "{ \"Value\":2, \"Errornumber\":0, \"Errormessage\":\"\" }";
  jsonretstr = "{\"Value\":" + String(1) + ",\"Errornumber\":0,\"Errormessage\":\"\" }";

  // sendreply builds http header, sets content type, and then sends jsonretstr
  sendreply(NORMALWEBPAGE, JSONPAGETYPE, jsonretstr);
}

// ----------------------------------------------------------------------
// get_description()
// ----------------------------------------------------------------------
void get_description() {
  // GET "/api/v1/observingconditions/0/description?ClientID=1&ClientTransactionID=1234" -H  "accept: application/json"
  // {  "Value": "string",  "Errornumber": 0,  "Errormessage": "string" }

  String jsonretstr = "";

  _ASCOMServerTransactionID++;
  _ASCOMErrorNumber = 0;
  _ASCOMErrorMessage = "";
  getURLParameters();

  jsonretstr = "{\"Value\":" + String(ASCOMDESCRIPTION) + ",\"Errornumber\":0,\"Errormessage\":\"\" }";

  // sendreply builds http header, sets content type, and then sends jsonretstr
  sendreply(NORMALWEBPAGE, JSONPAGETYPE, jsonretstr);
}

// ----------------------------------------------------------------------
// get_name()
// ----------------------------------------------------------------------
void get_name() {
  // curl -X GET "192.168.2.128:4040/api/v1/observingconditions/0/name?ClientID=1&ClientTransactionID=1234" -H  "accept: application/json"
  // {"Value":"myFP2ESPASCOMR","ClientID":1,"ClientTransactionID":1234,"ServerTransactionID":2,"Errornumber":"0","Errormessage":""myFP2ESPASCOMR""}

  String jsonretstr = "";

  _ASCOMServerTransactionID++;
  _ASCOMErrorNumber = 0;
  _ASCOMErrorMessage = "";
  getURLParameters();
  // addclientinfo adds clientid, clienttransactionid, servertransactionid, errornumber, errormessage and terminating }
  jsonretstr = "{\"Value\":" + String(ASCOMNAME) + ",\"Errornumber\":0,\"Errormessage\":\"\" }";

  // sendreply builds http header, sets content type, and then sends jsonretstr
  sendreply(NORMALWEBPAGE, JSONPAGETYPE, jsonretstr);
}

// ----------------------------------------------------------------------
// get_driverinfo()
// ----------------------------------------------------------------------
void get_driverinfo() {
  // curl -X GET "/api/v1/observingconditions/0/driverinfo?ClientID=1&ClientTransactionID=1234" -H  "accept: application/json"
  // {  "Value": "string",  "Errornumber": 0,  "Errormessage": "string" }
  String jsonretstr = "";

  _ASCOMServerTransactionID++;
  _ASCOMErrorNumber = 0;
  _ASCOMErrorMessage = "";
  getURLParameters();
  // addclientinfo adds clientid, clienttransactionid, servertransactionid, errornumber, errormessage and terminating }
  //jsonretstr = "{\"Value\":\"" + String(ASCOMDRIVERINFO) + "\",\"Errornumber\":0,\"Errormessage\":\"\" }";
  jsonretstr = "{\"Value\":" + String(ASCOMDRIVERINFO) + ",\"Errornumber\":0,\"Errormessage\":\"\" }";

  // sendreply builds http header, sets content type, and then sends jsonretstr
  sendreply(NORMALWEBPAGE, JSONPAGETYPE, jsonretstr);
}

// ----------------------------------------------------------------------
// get_driverversion()
// ----------------------------------------------------------------------
void get_driverversion() {
  // curl -X GET "/api/v1/observingconditions/0/driverversion?ClientID=1&ClientTransactionID=1234" -H  "accept: application/json"
  // {  "Value": "string",  "Errornumber": 0,  "Errormessage": "string" }

  String jsonretstr = "";

  _ASCOMServerTransactionID++;
  _ASCOMErrorNumber = 0;
  _ASCOMErrorMessage = "";
  getURLParameters();
  // addclientinfo adds clientid, clienttransactionid, servertransactionid, errornumber, errormessage and terminating }
  jsonretstr = "{\"Value\":\"" + String(program_version) + "\",\"Errornumber\":0,\"Errormessage\":\"\" }";

  // sendreply builds http header, sets content type, and then sends jsonretstr
  sendreply(NORMALWEBPAGE, JSONPAGETYPE, jsonretstr);
}

// ----------------------------------------------------------------------
// set_connected()
// ----------------------------------------------------------------------
void set_connected() {
  // curl -X PUT 192.168.2.128:4040/api/v1/observingconditions/0/connected -H  "accept: application/json" -H  "Content-Type: application/x-www-form-urlencoded" -d "Connected=true&ClientID=1&ClientTransactionID=2"
  // response { "Errornumber":0, "Errormessage":"" }

  String jsonretstr = "";

  _ASCOMServerTransactionID++;
  _ASCOMErrorNumber = 0;
  _ASCOMErrorMessage = "";
  getURLParameters();  // checks connected param and sets _ASCOMConnectedState
  jsonretstr = "{ \"Errornumber\":0, \"Errormessage\":\"\" }";

  // sendreply builds http header, sets content type, and then sends jsonretstr
  sendreply(NORMALWEBPAGE, JSONPAGETYPE, jsonretstr);
}



// ----------------------------------------------------------------------
// addclientinfo
// Adds client info to reply
// ----------------------------------------------------------------------
String addclientinfo(String str) {
  String str1 = str;
  // add clientid
  str1 = str1 + "\"ClientID\":" + String(_ASCOMClientID) + ",";
  // add clienttransactionid
  str1 = str1 + "\"ClientTransactionID\":" + String(_ASCOMClientTransactionID) + ",";
  // add ServerTransactionID
  str1 = str1 + "\"ServerTransactionID\":" + String(_ASCOMServerTransactionID) + ",";
  // add errornumber
  str1 = str1 + "\"ErrorNumber\":" + String(_ASCOMErrorNumber) + ",";
  // add errormessage
  str1 = str1 + "\"ErrorMessage\":\"" + _ASCOMErrorMessage + "\"}";
  return str1;
}

void get_supportedactions() {
  String jsonretstr = "";

  _ASCOMServerTransactionID++;
  _ASCOMErrorNumber = 0;
  _ASCOMErrorMessage = "";
  // get clientID and clienttransactionID
  getURLParameters();
  jsonretstr = "{\"Value\": [\"AveragePeriod\",\"DewPoint\",\"Humidity\",\"RainRate\",\"SkyQuality\",\"SkyTemperature\",\"Temperature\",\"WindSpeed\"]," + addclientinfo(jsonretstr);

  sendreply(NORMALWEBPAGE, JSONPAGETYPE, jsonretstr);
}

// ----------------------------------------------------------------------
// sendreply
// send a reply to client
// ----------------------------------------------------------------------
void sendreply(int replycode, String contenttype, String jsonstr) {
  // ascomserver.send builds the http header, jsonstr will be in the body
  _ascomserver->send(replycode, contenttype, jsonstr);
}

void get_averageperiod() {
  String jsonretstr = "";

  _ASCOMServerTransactionID++;
  _ASCOMErrorNumber = 0;
  _ASCOMErrorMessage = "";
  getURLParameters();
  float time = (float)averageperiod / 1000 / 60 / 60;
  // addclientinfo adds clientid, clienttransactionid, servertransactionid, errornumber, errormessage and terminating }
  jsonretstr = "{\"Value\":" + String(averageperiod) + ",\"Errornumber\":0,\"Errormessage\":\"\" }";

  // sendreply builds http header, sets content type, and then sends jsonretstr
  sendreply(NORMALWEBPAGE, JSONPAGETYPE, jsonretstr);
}

void set_averageperiod() {
  String jsonretstr = "";

  _ASCOMServerTransactionID++;
  _ASCOMErrorNumber = 0;
  _ASCOMErrorMessage = "";
  getURLParameters();  // checks connected param and sets _ASCOMConnectedState

  if (averageperiod < 0) {
    //Serial.print("Setting Average period to ");
    //Serial.println(averageperiod);
    averageperiod = 0;
    _ASCOMErrorNumber = ASCOMNOTIMPLEMENTED;
    _ASCOMErrorMessage = T_NOTIMPLEMENTED;
    jsonretstr = "{ \"ErrorNumber\":" + String("1025") + ",\"ErrorMessage\":\"" + String("Bad input : no negative number allowed") + "\" }";

    // sendreply builds http header, sets content type, and then sends jsonretstr
    sendreply(NORMALWEBPAGE, JSONPAGETYPE, jsonretstr);
    return;
  } else jsonretstr = "{ \"ErrorNumber\":0, \"ErrorMessage\":\"\" }";

  // sendreply builds http header, sets content type, and then sends jsonretstr
  sendreply(NORMALWEBPAGE, JSONPAGETYPE, jsonretstr);
}

void get_dewpoint() {
  String jsonretstr = "";

  _ASCOMServerTransactionID++;
  _ASCOMErrorNumber = 0;
  _ASCOMErrorMessage = "";
  getURLParameters();
  // addclientinfo adds clientid, clienttransactionid, servertransactionid, errornumber, errormessage and terminating }
  jsonretstr = "{\"Value\":" + String(dewpoint) + ",\"Errornumber\":0,\"Errormessage\":\"\" }";

  // sendreply builds http header, sets content type, and then sends jsonretstr
  sendreply(NORMALWEBPAGE, JSONPAGETYPE, jsonretstr);
}

void get_humidity() {
  String jsonretstr = "";

  _ASCOMServerTransactionID++;
  _ASCOMErrorNumber = 0;
  _ASCOMErrorMessage = "";
  getURLParameters();
  // addclientinfo adds clientid, clienttransactionid, servertransactionid, errornumber, errormessage and terminating }
  jsonretstr = "{\"Value\":" + String(humidity) + ",\"Errornumber\":0,\"Errormessage\":\"\" }";

  // sendreply builds http header, sets content type, and then sends jsonretstr
  sendreply(NORMALWEBPAGE, JSONPAGETYPE, jsonretstr);
}

void get_rainrate() {
  String jsonretstr = "";

  _ASCOMServerTransactionID++;
  _ASCOMErrorNumber = 0;
  _ASCOMErrorMessage = "";
  getURLParameters();
  // addclientinfo adds clientid, clienttransactionid, servertransactionid, errornumber, errormessage and terminating }
  jsonretstr = "{\"Value\":" + String(rainrate) + ",\"Errornumber\":0,\"Errormessage\":\"\" }";

  // sendreply builds http header, sets content type, and then sends jsonretstr
  sendreply(NORMALWEBPAGE, JSONPAGETYPE, jsonretstr);
}

void get_skytemperature() {
  String jsonretstr = "";

  _ASCOMServerTransactionID++;
  _ASCOMErrorNumber = 0;
  _ASCOMErrorMessage = "";
  getURLParameters();
  // addclientinfo adds clientid, clienttransactionid, servertransactionid, errornumber, errormessage and terminating }
  jsonretstr = "{\"Value\":" + String(skytemperature) + ",\"Errornumber\":0,\"Errormessage\":\"\" }";

  // sendreply builds http header, sets content type, and then sends jsonretstr
  sendreply(NORMALWEBPAGE, JSONPAGETYPE, jsonretstr);
}

void get_temperature() {
  String jsonretstr = "";

  _ASCOMServerTransactionID++;
  _ASCOMErrorNumber = 0;
  _ASCOMErrorMessage = "";
  getURLParameters();
  // addclientinfo adds clientid, clienttransactionid, servertransactionid, errornumber, errormessage and terminating }
  jsonretstr = "{\"Value\":" + String(temperature) + ",\"Errornumber\":0,\"Errormessage\":\"\" }";

  // sendreply builds http header, sets content type, and then sends jsonretstr
  sendreply(NORMALWEBPAGE, JSONPAGETYPE, jsonretstr);
}

void get_skyquality() {
  String jsonretstr = "";

  _ASCOMServerTransactionID++;
  _ASCOMErrorNumber = 0;
  _ASCOMErrorMessage = "";
  getURLParameters();
  // addclientinfo adds clientid, clienttransactionid, servertransactionid, errornumber, errormessage and terminating }
  //jsonretstr = "{\"Value\":" + String(mySQMreading) + ",\"Errornumber\":0,\"Errormessage\":\"\" }";
  jsonretstr = "{\"Value\":" + String(skyquality) + ",\"Errornumber\":0,\"Errormessage\":\"\" }";
  // sendreply builds http header, sets content type, and then sends jsonretstr
  sendreply(NORMALWEBPAGE, JSONPAGETYPE, jsonretstr);
}

void get_windspeed() {
  String jsonretstr = "";

  _ASCOMServerTransactionID++;
  _ASCOMErrorNumber = 0;
  _ASCOMErrorMessage = "";
  getURLParameters();
  // addclientinfo adds clientid, clienttransactionid, servertransactionid, errornumber, errormessage and terminating }
  jsonretstr = "{\"Value\":" + String(windspeed) + ",\"ErrorNumber\":0,\"ErrorMessage\":\"\" }";

  // sendreply builds http header, sets content type, and then sends jsonretstr
  sendreply(NORMALWEBPAGE, JSONPAGETYPE, jsonretstr);
}

void get_winddirection() {
  String jsonretstr = "";

  _ASCOMServerTransactionID++;
  _ASCOMErrorNumber = 0;
  _ASCOMErrorMessage = "";
  getURLParameters();
  // addclientinfo adds clientid, clienttransactionid, servertransactionid, errornumber, errormessage and terminating }
  jsonretstr = "{\"Value\":" + String(0.0) + ",\"ErrorNumber\":0,\"ErrorMessage\":\"\" }";

  // sendreply builds http header, sets content type, and then sends jsonretstr
  sendreply(NORMALWEBPAGE, JSONPAGETYPE, jsonretstr);
}

// ----------------------------------------------------------------------
// get_notfound()
//
// ----------------------------------------------------------------------
void get_notfound() {
  String message = "err not found ";
  String jsonretstr = "";

  message += "URI: ";
  message += _ascomserver->uri();
  message += "\nMethod: ";
  if (_ascomserver->method() == HTTP_GET) {
    message += "GET";
  } else if (_ascomserver->method() == HTTP_POST) {
    message += "POST";
  } else if (_ascomserver->method() == HTTP_PUT) {
    message += "PUT";
  } else if (_ascomserver->method() == HTTP_DELETE) {
    message += "DELETE";
  } else {
    message += "UNKNOWN_METHOD: " + _ascomserver->method();
  }
  message += "\nArguments: ";
  message += _ascomserver->args();
  message += "\n";
  for (uint8_t i = 0; i < _ascomserver->args(); i++) {
    message += " " + _ascomserver->argName(i) + ": " + _ascomserver->arg(i) + "\n";
  }

  _ASCOMErrorNumber = ASCOMNOTIMPLEMENTED;
  _ASCOMErrorMessage = T_NOTIMPLEMENTED;
  _ASCOMServerTransactionID++;
  jsonretstr = "{" + addclientinfo(jsonretstr);

  sendreply(NORMALWEBPAGE, JSONPAGETYPE, jsonretstr);
}

void get_sensordescription() {
  String jsonretstr = "";

  _ASCOMServerTransactionID++;
  _ASCOMErrorNumber = 0;
  _ASCOMErrorMessage = "";
  getURLParameters();
  if (sensordescription != "") {
    jsonretstr = "{\"Value\":" + sensordescription + ",\"ErrorNumber\":0,\"ErrorMessage\":\"\" }";

    // sendreply builds http header, sets content type, and then sends jsonretstr
    sendreply(NORMALWEBPAGE, JSONPAGETYPE, jsonretstr);
  } else {
    get_notfound();
  }
}

void get_timesincelastupdate() {
  String jsonretstr = "";

  _ASCOMServerTransactionID++;
  _ASCOMErrorNumber = 0;
  _ASCOMErrorMessage = "";
  getURLParameters();
  if (timesincelastupdate != -1) {
    // addclientinfo adds clientid, clienttransactionid, servertransactionid, errornumber, errormessage and terminating }
    jsonretstr = "{\"Value\":" + String(timesincelastupdate) + ",\"ErrorNumber\":0,\"ErrorMessage\":\"\" }";

    // sendreply builds http header, sets content type, and then sends jsonretstr
    sendreply(NORMALWEBPAGE, JSONPAGETYPE, jsonretstr);
  } else {
    get_notfound();
  }
}


