#include "Definitions.h"
#include <Arduino.h>

const char* program_version = "0.2";


byte adr[25] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 15, 19, 23, 27, 31, 35, 39, 43, 47, 67, 87, 107, 127 };

String sensornamelist[13] = {
  "CloudCover",
  "DewPoint",
  "Humidity",
  "Pressure",
  "RainRate",
  "SkyBrightness",
  "SkyQuality",
  "StarFWHM",
  "SkyTemperature",
  "Temperature",
  "WindDirection",
  "WindGust",
  "WindSpeed"
};

String sensordescriptionlist[13] = {
  "",
  "\"Sensirion/DFrobot, SEN0385-SHT31\"",
  "\"Sensirion/DFrobot, SEN0385-SHT31\"",
  "",
  "\"Hydreon, RG11\"",
  "",
  "\"AMS, TLS237+Ircut\"",
  "",
  "\"Melexis, MLX90614-BAA193DB12\"",
  "\"Sensirion/DFrobot, SEN0385-SHT31\"",
  "\"Unknown, 2.4 km/h/pulse\"",
  "",
  "\"Unknown, 2.4 km/h/pulse\""
};

//-1 if value is not implemented; 10 otherwise
double timesincelastupdatelist[13] = {
  -1,
  10,
  10,
  -1,
  10,
  -1,
  10,
  -1,
  10,
  10,
  10,
  -1,
  10
};

/*
 * Server Index Page
 */
 
const char* serverIndex = 
"<script src='https://ajax.googleapis.com/ajax/libs/jquery/3.2.1/jquery.min.js'></script>"
"<form method='POST' action='#' enctype='multipart/form-data' id='upload_form'>"
   "<input type='file' name='update'>"
        "<input type='submit' value='Update'>"
    "</form>"
 "<div id='prg'>progress: 0%</div>"
 "<script>"
  "$('form').submit(function(e){"
  "e.preventDefault();"
  "var form = $('#upload_form')[0];"
  "var data = new FormData(form);"
  " $.ajax({"
  "url: '/update',"
  "type: 'POST',"
  "data: data,"
  "contentType: false,"
  "processData:false,"
  "xhr: function() {"
  "var xhr = new window.XMLHttpRequest();"
  "xhr.upload.addEventListener('progress', function(evt) {"
  "if (evt.lengthComputable) {"
  "var per = evt.loaded / evt.total;"
  "$('#prg').html('progress: ' + Math.round(per*100) + '%');"
  "}"
  "}, false);"
  "return xhr;"
  "},"
  "success:function(d, s) {"
  "console.log('success!')" 
 "},"
 "error: function (a, b, c) {"
 "}"
 "});"
 "});"
 "</script>";

const char* host = "esp32";

// You put the html code of the webpage here in a single long string. 
//The human readable html code is available in the file config.html
//If you change anything to config.html, just copy the content in a site like https://www.textfixer.com/html/compress-html-compression.php, compress it and copy the string here between the ""
//Future revisions will maybe use config.html directly and store it in the SPIFFS partition of the esp32
const char* webpage ="<!DOCTYPE html><html lang='en' class='js-focus-visible'><meta charset='utf-8' /><title>Web Page Update Demo</title> <style> table { position: relative; width:100%; border-spacing: 0px; } tr { border: 1px solid white; font-family: 'Verdana', 'Arial', sans-serif; font-size: 10px; } th { height: 20px; padding: 3px 15px; background-color: #343a40; color: #FFFFFF !important; } td { height: 20px; padding: 3px 15px; } .tabledata { font-size: 12px; position: relative; padding-left: 5px; padding-top: 5px; height: 25px; border-radius: 5px; color: #FFFFFF; line-height: 20px; transition: all 200ms ease-in-out; background-color: #c8c8c8; } .fanrpmslider { width: 30%; height: 55px; outline: none; height: 25px; } .bodytext { font-family: 'Verdana', 'Arial', sans-serif; font-size: 12px; text-align: left; font-weight: light; border-radius: 5px; display:inline; } .navbar { width: 100%; height: 50px; margin: 0; padding: 10px 0px; background-color: #FFF; color: #000000; border-bottom: 5px solid #293578; } .fixed-top { position: fixed; top: 0; right: 0; left: 0; z-index: 1030; } .navtitle { float: left; height: 50px; font-family: 'Verdana', 'Arial', sans-serif; font-size: 25px; font-weight: bold; line-height: 50px; padding-left: 20px; } .navheading { position: fixed; left: 50%; height: 50px; font-family: 'Verdana', 'Arial', sans-serif; font-size: 15px; font-weight: bold; line-height: 20px; padding-right: 20px; } .navdata { justify-content: flex-end; position: fixed; left: 55%; height: 50px; font-family: 'Verdana', 'Arial', sans-serif; font-size: 15px; font-weight: bold; line-height: 20px; padding-right: 20px; } .category { font-family: 'Verdana', 'Arial', sans-serif; font-weight: bold; font-size: 20px; line-height: 50px; padding: 0px 10px 0px 0px; color: #000000; }.category2 { font-family: 'Verdana', 'Arial', sans-serif; font-weight: bold; font-size: 15px; line-height: 50px; padding: 0px 10px 0px 10px; color: #000000; }.normaltext { font-family: 'Verdana', 'Arial', sans-serif; font-weight: normal; font-size: 12px; } .heading { font-family: 'Verdana', 'Arial', sans-serif; font-weight: normal; font-size: 14px; text-align: left; } .btn { background-color: #444444; border: none; color: white; padding: 10px 20px; text-align: center; text-decoration: none; display: inline-block; font-size: 16px; margin: 4px 2px; cursor: pointer; } .foot { font-family: 'Verdana', 'Arial', sans-serif; font-size: 10px; position: relative; height: 30px; text-align: center; color: #AAAAAA; line-height: 20px; } .container { max-width: 1000px; margin: 0 auto; } table tr:first-child th:first-child { border-top-left-radius: 5px; } table tr:first-child th:last-child { border-top-right-radius: 5px; } table tr:last-child td:first-child { border-bottom-left-radius: 5px; } table tr:last-child td:last-child { border-bottom-right-radius: 5px; } </style> <body style='background-color: #efefef' onload='process()'> <header> <div class='navbar fixed-top'> <div class='container'> <div class='navtitle'>CSSB Monitor</div> <div class='navdata' id = 'date'>mm/dd/yyyy</div> <div class='navheading'>DATE</div><br> <div class='navdata' id = 'time'>00:00:00</div> <div class='navheading'>TIME</div> </div> </div> </header> <main class='container' style='margin-top:70px'> <div class='category'>Sensor Readings</div> <div style='border-radius: 10px !important;'> <table style='width:50%'> <colgroup> <col span='1' style='background-color:rgb(230,230,230); width: 20%; color:#000000 ;'> <col span='1' style='background-color:rgb(200,200,200); width: 15%; color:#000000 ;'> <col span='1' style='background-color:rgb(180,180,180); width: 15%; color:#000000 ;'> </colgroup> <col span='2'style='background-color:rgb(0,0,0); color:#FFFFFF'> <col span='2'style='background-color:rgb(0,0,0); color:#FFFFFF'> <col span='2'style='background-color:rgb(0,0,0); color:#FFFFFF'> <tr> <th colspan='1'><div class='heading'>Sensor</div></th> <th colspan='1'><div class='heading'>Value</div></th> </tr> <tr> <td><div class='bodytext'>Temperature (°C)</div></td> <td><div class='tabledata' id = 'tmp'></div></td> </tr> <tr> <td><div class='bodytext'>Humidity (%)</div></td> <td><div class='tabledata' id = 'hum'></div></td> </tr> <tr> <td><div class='bodytext'>Dew Point (°C)</div></td> <td><div class='tabledata' id = 'dew'></div></td> </tr> <tr> <td><div class='bodytext'>Sky Temperature (°C)</div></td> <td><div class='tabledata' id = 'cloud'></div></td> </tr> <tr> <td><div class='bodytext'>Raining ?</div></td> <td><div class='tabledata' id = 'rain'></div></td> </tr> <tr> <td><div class='bodytext'>Sky Quality Meter (mag/arcsec²)</div></td> <td><div class='tabledata' id = 'sqm'></div></td> </tr> <tr> <td><div class='bodytext'>Wind (km/h)</div></td> <td><div class='tabledata' id = 'wind'></div></td> </tr> </table> </div><br> <div class='category'>Sensor Parameters</div> <br><div class='category2'>--- Sampling ---</div><form> <input type='text' id='sampling' name='sampling'> <label for='sampling'> (seconds) Sampling time Resolution (min 2s)</label><br> <input type='text' id='averaging' name='averaging'> <label for='averaging'> (seconds) Averaging period (minimum 30s) </label><br></form> <br> <div class='category2'>--- Cloud Sensor Compensation model ---</div><form> <input type='text' id='k1' name='k1'> <label for='k1'> K1 (0-255)</label><br> <input type='text' id='k2' name='k2'> <label for='k2'> K2 (0-255)</label><br> <input type='text' id='k3' name='k3'> <label for='k3'> K3 (0-255)</label><br> <input type='text' id='k4' name='k4'> <label for='k4'> K4 (0-255)</label><br> <input type='text' id='k5' name='k5'> <label for='k5'> K5 (0-255)</label><br> <input type='text' id='k6' name='k6'> <label for='k6'> K6 (0-255)</label><br> <input type='text' id='k7' name='k7'> <label for='k7'> K7 (0-255)</label><br><br> <label for='flagCloud'> Ambiant Temperature compensation : internal sensor </label> <input type='checkbox' id='flagCloud' name='flagCloud' value='flag'> <label for='flagCloud'> (unchecked: measured with external SHT31)</label><br></form> <br> <div class='category2'>--- Sky Quality Meter Calibration ---</div><form> <input type='text' id='aFactor' name='aFactor'> <label for='aFactor'> : A factor</label><br> <input type='text' id='darkFreq' name='darkFreq'> <label for='darkFreq'> : Dark current frequency (Hz)</label><br></form> <br> <div class='category2'>--- Anemometer Calibration ---</div><form> <input type='text' id='windFactor' name='windFactor'> <label for='windFactor'> Wind Speed Factor</label><br></form> <br> <div class='category'>Network Parameters</div> <br> <div class='category2'>--- Thingspeak ---</div><form> <input type='text' id='APIkey' name='APIkey'> <label for='APIkey'> Read API key</label><br> <input type='text' id='channelid' name='channelid'> <label for='channelid'> Channel ID</label><br></form> <div class='category2'>--- WiFi ---</div><form> <input type='text' id='ssid' name='ssid'> <label for='ssid'> SSID</label><br> <input type='text' id='pwd' name='pwd'> <label for='pwd'> Password</label><br> <form> <br> <div class='category'>Safety Monitor</div> <br> <main class='container' style='margin-top:70px'> <div class='category'>Safety Conditions</div> <div style='border-radius: 10px !important;'> <table style='width:50%'> <colgroup> <col span='1' style='background-color:rgb(230,230,230); width: 40%; color:#000000 ;'> <col span='1' style='background-color:rgb(200,200,200); width: 15%; color:#000000 ;'> <col span='1' style='background-color:rgb(180,180,180); width: 25%; color:#000000 ;'> </colgroup> <col span='2'style='background-color:rgb(0,0,0); color:#FFFFFF'> <col span='2'style='background-color:rgb(0,0,0); color:#FFFFFF'> <col span='2'style='background-color:rgb(0,0,0); color:#FFFFFF'> </colgroup> <col span='3'style='background-color:rgb(0,0,0); color:#FFFFFF'> <col span='3'style='background-color:rgb(0,0,0); color:#FFFFFF'> <col span='3'style='background-color:rgb(0,0,0); color:#FFFFFF'> <tr> <th colspan='1'><div class='heading'>Condition</div></th> <th colspan='1'><div class='heading'>Value</div></th><th colspan='1'><div class='heading'>Is Safe ?</div></th> </tr> <tr> <td><div class='bodytext'>Sky Temperature <=</div></td> <td><input type='text' id='cloudcond' name='cloudcond' placeholder=''</td><td><div class='bodytext' id = 'cloudsafe'> No </div></td> </tr> <tr> <td><div class='bodytext'>Rain =</div></td> <td><input type='text' id='raincond' name='raincond' placeholder=''</td><td><div class='bodytext' id = 'rainsafe'> No </div></td> </tr> <tr> <td><div class='bodytext'>Wind Speed <=</div></td> <td><input type='text' id='windcond' name='windcond' placeholder=''</td><td><div class='bodytext' id = 'windsafe'> No </div></td> </tr> </table> </div></form><span style='color: #003366;'><p><button type='button' id='BTN_GET'>Get parameters from ESP32</button><br><button type='button' id='BTN_SEND_BACK'>Save to EEPROM</button></p></span></body><script> var Socket; var cloudval; var rainval; var windval; var starting=true; document.getElementById('BTN_SEND_BACK').addEventListener('click', button_send_back); document.getElementById('BTN_GET').addEventListener('click', button_get); function isEmpty(str) { if(str==null) return true; return !str.trim().length;} function isNumber(value) { if(isEmpty(value))return false; else if(Number.isNaN(Number(value))) return false; else return true;} function init() { Socket = new WebSocket('ws://' + window.location.hostname + ':81/'); Socket.onmessage = function(event) { processCommand(event); }; } function button_send_back() {var field_not_valid=false;if (isNumber(sampling.value)==false) field_not_valid=true;if (isNumber(averaging.value)==false) field_not_valid=true;if (isNumber(k1.value)==false) field_not_valid=true;if (isNumber(k2.value)==false) field_not_valid=true;if (isNumber(k3.value)==false) field_not_valid=true;if (isNumber(k4.value)==false) field_not_valid=true;if (isNumber(k5.value)==false) field_not_valid=true;if (isNumber(k6.value)==false) field_not_valid=true;if (isNumber(k7.value)==false) field_not_valid=true;if (isNumber(aFactor.value)==false) field_not_valid=true;if (isNumber(darkFreq.value)==false) field_not_valid=true;if (isNumber(windFactor.value)==false) field_not_valid=true;if (isEmpty(APIkey.value)) field_not_valid=true;if (isNumber(channelid.value)==false ) field_not_valid=true;if (isEmpty(ssid.value)) field_not_valid=true;if (isEmpty(pwd.value)) field_not_valid=true;if (isEmpty(cloudcond.value)) field_not_valid=true;if (isEmpty(raincond.value)) field_not_valid=true;if (isEmpty(windcond.value)) field_not_valid=true;if (field_not_valid==true){button_get();return ;} var flag_state = '';if(document.getElementById('flagCloud').checked == true) { flag_state = '1';} else if(document.getElementById('flagCloud').checked == false) { flag_state = '0';} var msg = {request: 'SEND_BACK',_samplingperiod: sampling.value,_averageperiod: averaging.value,_k1: k1.value,_k2: k2.value,_k3: k3.value,_k4: k4.value,_k5: k5.value,_k6: k6.value,_k7: k7.value,_flag: flag_state,_aFactor: aFactor.value,_darkFreq: darkFreq.value,_windFactor: windFactor.value,_APIkey: APIkey.value,_channelid: channelid.value,_ssid: ssid.value,_pwd: pwd.value,_cloudcond: cloudcond.value,_raincond: raincond.value,_windcond: windcond.value};Socket.send(JSON.stringify(msg)); } function button_get() { var msg = {request: 'GET_EEPROM'};Socket.send(JSON.stringify(msg)); } function processCommand(event) { var dt = new Date();var obj = JSON.parse(event.data);if(obj.answer == 'SEND_READINGS'){document.getElementById('tmp').innerHTML = obj.tmp;document.getElementById('hum').innerHTML = obj.hum;document.getElementById('dew').innerHTML = obj.dew;document.getElementById('cloud').innerHTML = obj.cloud;cloudval=parseInt(obj.cloud);document.getElementById('rain').innerHTML = obj.rain;rainval=parseInt(obj.rain);document.getElementById('sqm').innerHTML = obj.sqm;document.getElementById('wind').innerHTML = obj.wind;windval=parseInt(obj.wind); document.getElementById('time').innerHTML = dt.toLocaleTimeString(); document.getElementById('date').innerHTML = dt.toLocaleDateString(); console.log(obj.tmp);console.log(obj.hum);console.log(obj.dew);console.log(obj.cloud);console.log(obj.rain);console.log(obj.sqm);console.log(obj.wind);if(cloudval<= parseInt(document.getElementById('cloudcond').value)) document.getElementById('cloudsafe').innerHTML = 'Safe'; else document.getElementById('cloudsafe').innerHTML = 'Unsafe'; if(rainval== 0) document.getElementById('rainsafe').innerHTML = 'Safe'; else document.getElementById('rainsafe').innerHTML = 'Unsafe'; if(windval<= parseFloat(document.getElementById('windcond').value)) document.getElementById('windsafe').innerHTML = 'Safe'; else document.getElementById('windsafe').innerHTML = 'Unsafe'; if(starting==true){button_get();starting=false;}}else if(obj.answer == 'SEND_EEPROM'){ var flag_state = '';if(obj.flag == '1') { document.getElementById('flagCloud').checked = true;} else if(obj.flag == '0') { document.getElementById('flagCloud').checked = false;}document.getElementById('sampling').value = obj.sampling;document.getElementById('averaging').value = obj.averaging;document.getElementById('k1').value = obj.k1;document.getElementById('k2').value = obj.k2;document.getElementById('k3').value = obj.k3;document.getElementById('k4').value = obj.k4;document.getElementById('k5').value = obj.k5;document.getElementById('k6').value = obj.k6;document.getElementById('k7').value = obj.k7;document.getElementById('aFactor').value = obj.aFactor;document.getElementById('darkFreq').value = obj.darkFreq;document.getElementById('windFactor').value = obj.windFactor;document.getElementById('APIkey').value = obj.APIkey;document.getElementById('channelid').value = obj.channelid;document.getElementById('ssid').value = obj.ssid;document.getElementById('pwd').value = obj.pwd;document.getElementById('cloudcond').value = obj.cloudcond;document.getElementById('raincond').value = obj.raincond;document.getElementById('windcond').value = obj.windcond;} } window.onload = function(event) { init(); }</script></html>";
