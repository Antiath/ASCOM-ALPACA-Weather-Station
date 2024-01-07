/*----------------------------------------------------------------------
Clear Sky Sensor Box V2 - Daughter board Teensy 3.2 Firmware
----------------------------------------------------------------------

----------------------------------------------------------------------
Description
----------------------------------------------------------------------

Code for dealing with the SQM sensor and anemometer.
I couldn't make those work on the esp32 alongside the other stuff.
1) The two frequency counter libraries used here are not supported on the esp32.
2) Freq counters made for the esp32 were very unstable on the main firmware of the esp32 and prone to crash.
So to solve this, I exported those two sensors on a Teensy 3.2 and the measurements are sent through the i2c bus.

All the libraries used here are also compatible with Arduino AVR boards but the wiring will be different.

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


#include <FreqMeasure.h>
#include <FreqCount.h>
#include <elapsedMillis.h>
#include <Wire.h>


elapsedMillis timeout;

float frequency;
int count = 0;
int sqm = 0;
bool sqmstate = false;
byte byteArray[5];

byte RxByte;
byte TxByte = 0;
String cmd;

void I2C_TxHandler(void) {
  String w = String(frequency);
  String s = String(sqm);
  String b = w;
  b.concat('+');
  b.concat(s);
  b.concat('#');

  char sendbuff[32];
  b.toCharArray(sendbuff, 32);

  Serial.println(frequency);
  Serial.println(sqm);
  //Serial.println(sendbuff);
  Wire.write(sendbuff);
}

void setup() {
  Serial.begin(57600);
  FreqMeasure.begin();
  timeout = 0;

  Wire.begin(0x55);  // Initialize I2C (Slave Mode: address=0x55 )
  //Wire.onReceive(I2C_RxHandler);
  Wire.onRequest(I2C_TxHandler);
}

double sum = 0;
int t;
void loop() {

  if (FreqMeasure.available() && sqmstate == false) {
    // average several reading together
    t=FreqMeasure.read();
    if((t>100000) && (t<20000000)){
    //Serial.println(t);
    sum = sum + t;
    count = count + 1;
    if (count > 10) {
        //  Serial.println(sum / count);
      frequency = FreqMeasure.countToFrequency(sum / count);
      //Serial.println(frequency);
      sum = 0;
      count = 0;
      timeout = 0;
      sqmstate = true;
      //FreqMeasure.end();
      delay(100);
      FreqCount.begin(1000);
    }
    }
  }
  if (timeout > 10000) {
    frequency = 0;
    timeout = 0;
    sqmstate = true;
    count = 0;
    //FreqMeasure.end();
    delay(100);
    FreqCount.begin(1000);
  }

  if (FreqCount.available() && sqmstate == true) {
    sqm = FreqCount.read();
    FreqCount.end();
    sqmstate = false;
    //FreqMeasure.begin();
    delay(100);
  }
}