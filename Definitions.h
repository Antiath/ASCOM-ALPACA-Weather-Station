//-----------------------------------------------------------------------
// CSSBv2_ESP32 Definitions
// // Copyright (c) 2023 F. Mispelaer
//-----------------------------------------------------------------------
#ifndef _Definitions_h
#define _Definitions_h

#include <Arduino.h>
// ASCOM CONST VARS
#define ASCOMGUID "7e239e71-d304-4e7e-acda-3ff2e2b68515"
#define ASCOMMAXIMUMARGS 10
#define ASCOMNOTIMPLEMENTED 0x400

// ASCOM MESSAGES
#define ASCOMDESCRIPTION "\"Alpaca driver for CSSB stations\""
#define ASCOMDRIVERINFO "\"Clear Sky Sensor Box ALPACA SERVER (c) F. Mispelaer. 2023\""
#define ASCOMMANAGEMENTINFO "{\"ServerName\":\"CSSB_ESP32\",\"Manufacturer\":\"F. Mispelaer\",\"ManufacturerVersion\":\"v2.0\",\"Location\":\"Your ass\"}"
#define ASCOMNAME "\"CSSB_Server\""
#define ASCOMSERVERNOTFOUNDSTR "<html><head><title>ASCOM ALPACA Server</title></head><body><p>File system not started</p><p><a href=\"/setup/v1/focuser/0/setup\">Setup page</a></p></body></html>"
#define T_NOTIMPLEMENTED "not implemented"

// PORTS
#define ASCOMSERVERPORT 4040      // ASCOM Remote port
#define ASCOMDISCOVERYPORT 32227  // UDP


// SERIAL PORT
#define SERIALPORTSPEED 115200  // 9600, 14400, 19200, 28800, 38400, 57600, 115200

// defines for ASCOMSERVER, WEBSERVER
#define NORMALWEBPAGE 200
//#define FILEUPLOADSUCCESS 300
#define BADREQUESTWEBPAGE 400
#define NOTFOUNDWEBPAGE 404
#define INTERNALSERVERERROR 500

#define RAINPIN 26

extern const char* program_version;

extern byte adr[25];

extern String sensornamelist[13];
extern String sensordescriptionlist[13];
extern double timesincelastupdatelist[13];
extern const char* webpage;
#endif