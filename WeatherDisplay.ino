/**
 * The MIT License (MIT)
 *
 * Copyright (c) 2016 by Daniel Eichhorn
 * Copyright (c) 2016 by Fabrice Weinberg
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include <ESP8266HTTPClient.h>
#include <TimeLib.h>

// Include the correct display library
// For a connection via I2C using Wire include
#include <Wire.h>  // Only needed for Arduino 1.6.5 and earlier
#include <SSD1306.h> // alias for `#include "SSD1306Wire.h"`

// Include the UI lib
#include <OLEDDisplayUi.h>

// Include custom images
#include "images.h"
#include <NTPClientLib.h>
#include <ESP8266WiFi.h>

#include <Ticker.h>
#include "Secret.h" // File to add secret keys for Google an ForecastIO

// Initialize the OLED display using Wire library
SSD1306  display(0x3c, 0, 14);

OLEDDisplayUi ui ( &display );

int screenW = 128;
int screenH = 64;
int clockCenterX = screenW/2;
int clockCenterY = ((screenH-16)/2)+16;   // top yellow part is 16 px height
int clockRadius = 23;

// utility function for digital clock display: prints leading 0
String twoDigits(int digits){
  if(digits < 10) {
    String i = '0'+String(digits);
    return i;
  }
  else {
    return String(digits);
  }
}

void clockOverlay(OLEDDisplay *display, OLEDDisplayUiState* state) {

}

/* void analogClockFrame(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
//  ui.disableIndicator();

  // Draw the clock face
//  display->drawCircle(clockCenterX + x, clockCenterY + y, clockRadius);
  display->drawCircle(clockCenterX + x, clockCenterY + y, 2);
  //
  //hour ticks
  for( int z=0; z < 360;z= z + 30 ){
  //Begin at 0° and stop at 360°
    float angle = z ;
    angle = ( angle / 57.29577951 ) ; //Convert degrees to radians
    int x2 = ( clockCenterX + ( sin(angle) * clockRadius ) );
    int y2 = ( clockCenterY - ( cos(angle) * clockRadius ) );
    int x3 = ( clockCenterX + ( sin(angle) * ( clockRadius - ( clockRadius / 8 ) ) ) );
    int y3 = ( clockCenterY - ( cos(angle) * ( clockRadius - ( clockRadius / 8 ) ) ) );
    display->drawLine( x2 + x , y2 + y , x3 + x , y3 + y);
  }

  // display second hand
  float angle = second() * 6 ;
  angle = ( angle / 57.29577951 ) ; //Convert degrees to radians
  int x3 = ( clockCenterX + ( sin(angle) * ( clockRadius - ( clockRadius / 5 ) ) ) );
  int y3 = ( clockCenterY - ( cos(angle) * ( clockRadius - ( clockRadius / 5 ) ) ) );
  display->drawLine( clockCenterX + x , clockCenterY + y , x3 + x , y3 + y);
  //
  // display minute hand
  angle = minute() * 6 ;
  angle = ( angle / 57.29577951 ) ; //Convert degrees to radians
  x3 = ( clockCenterX + ( sin(angle) * ( clockRadius - ( clockRadius / 4 ) ) ) );
  y3 = ( clockCenterY - ( cos(angle) * ( clockRadius - ( clockRadius / 4 ) ) ) );
  display->drawLine( clockCenterX + x , clockCenterY + y , x3 + x , y3 + y);
  //
  // display hour hand
  angle = hour() * 30 + int( ( minute() / 12 ) * 6 )   ;
  angle = ( angle / 57.29577951 ) ; //Convert degrees to radians
  x3 = ( clockCenterX + ( sin(angle) * ( clockRadius - ( clockRadius / 2 ) ) ) );
  y3 = ( clockCenterY - ( cos(angle) * ( clockRadius - ( clockRadius / 2 ) ) ) );
  display->drawLine( clockCenterX + x , clockCenterY + y , x3 + x , y3 + y);
} */

void digitalClockFrame(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  String timenow = String(hour())+":"+twoDigits(minute())+":"+twoDigits(second());
  String datenow = twoDigits(day()) + "/" + twoDigits(month()) + "/" + String(year());
  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->setFont(ArialMT_Plain_24);
  display->drawString(clockCenterX + x , clockCenterY + y, timenow );
  display->setFont(ArialMT_Plain_16);
  display->drawString(clockCenterX + x, 16 + y, datenow);
}

float temperature = 0.0f;
float apparentTemp = 0.0f;
String forecastIOhost = "api.forecast.io"; // forecast.io host
IPAddress forecastIOip(69,164,223,38); 
#ifndef SECRET
String forecastIOkey = "YOUR FORECAST.IO SECRET KEY";
#endif
float lat = 0.0f;
float lon = 0.0f;
String forecastIOurl = "/forecast/";
String forecastIOparameters = "?units=si&exclude=minutely,hourly,daily,alerts,flags";
String forecastIOfingerprint = "89 83 31 6f 29 89 bd 18 0d 41 59 16 00 fe 57 98 77 d8 71 8e";
static WiFiClientSecure wifiClient;

String googleApisHost = "www.googleapis.com";
String googleApiUrl = "/geolocation/v1/geolocate";
#ifndef SECRET
String googleApiKey = "YOUR GOOGLE API KEY";
#endif
String googleApiFingerprint = "bb 2b 55 5b 37 a3 7e a8 75 d9 3b da 64 96 c7 f5 7d 6a 06 c4";
IPAddress googleApiIP(216,58,214,170);

String MACtoString(uint8_t* macAddress) {
	uint8_t mac[6];
	char macStr[18] = { 0 };
	sprintf(macStr, "%02X:%02X:%02X:%02X:%02X:%02X", macAddress[0], macAddress[1], macAddress[2], macAddress[3], macAddress[4], macAddress[5]);
	return  String(macStr);
}

// Function to get a list of surrounding WiFi signals in JSON format to get location via Google Location API
String getSurroundingWiFiJson() {
	String wifiArray = "[";

	int numWifi = WiFi.scanNetworks();
	for (int i = 0; i < numWifi; i++) {
		wifiArray += "{\"macAddress\":\"" + MACtoString(WiFi.BSSID(i)) + "\",";
		wifiArray += "\"signalStrength\":" + String(WiFi.RSSI(i)) + ",";
		wifiArray += "\"channel\":" + String(WiFi.channel(i)) + "}";
		if (i < (numWifi - 1)) {
			wifiArray += ",\n";
		}
	}
	WiFi.scanDelete();
	wifiArray += "]";
	Serial.println("WiFi list :\n" + wifiArray);
	return wifiArray;
}

// Calls Google Location API to get current location using surrounding WiFi signals inf
void getGeoFromWiFi() {
	String response = "";
	if (wifiClient.connect(googleApisHost.c_str(), 443)) {
		Serial.println("Connected: " + wifiClient.remoteIP().toString());
	}
	if (wifiClient.verify(googleApiFingerprint.c_str(), googleApisHost.c_str())) {
		Serial.println("certificate matches");
	}
	else {
		Serial.println("certificate doesn't match");
	}
	String body = "{\"wifiAccessPoints\":"+ getSurroundingWiFiJson() +"}";
	Serial.println("requesting URL: " + googleApiUrl + "?key=" + googleApiKey);
	String request = String("POST ") + googleApiUrl + "?key=" + googleApiKey + " HTTP/1.1\r\n" +
		"Host: " + googleApisHost + "\r\n" +
		"User-Agent: ESP8266\r\n" +
		"Content-Type:application/json\r\n" +
		"Content-Length:" + String(body.length()) + "\r\n" +
		"Connection: close\r\n\r\n" +
		body;
	Serial.println("request: \n" + request);
	wifiClient.print(request);
	Serial.println("request sent");
	while (wifiClient.connected()) {
		response += wifiClient.readString();
	}
	Serial.println("Got response:");
	if (response != "") {
		Serial.println(response);
		int index = response.indexOf("\"lat\":");
		if (index) {
			String tempStr = response.substring(index);
			//Serial.println(tempStr);
			tempStr = tempStr.substring(tempStr.indexOf(":") + 1, tempStr.indexOf(","));
			//Serial.println(tempStr);
			lat = tempStr.toFloat();
			Serial.println("\nLat: " + String(lat,7));
		}
		index = response.indexOf("\"lng\":");
		if (index) {
			String tempStr = response.substring(index);
			//Serial.println(tempStr);
			tempStr = tempStr.substring(tempStr.indexOf(":") + 1, tempStr.indexOf(","));
			//Serial.println(tempStr);
			lon = tempStr.toFloat();
			Serial.println("\nLon: " + String(lon,7));
		}
	}
}

// Gets temp info from ForecastIO API
void getTemperature() {
	//static int i = 0;
	String response = "";
	if (wifiClient.connect(forecastIOhost.c_str(), 443)) {
		Serial.println("Connected: " + wifiClient.remoteIP().toString());
	}
	if (wifiClient.verify(forecastIOfingerprint.c_str(),forecastIOhost.c_str())) {
		Serial.println("certificate matches");
	}
	else {
		Serial.println("certificate doesn't match");
	}
	Serial.println("requesting URL: " + forecastIOurl + "/key=KEY/" + String(lat,7) + "," + String(lon,7) + forecastIOparameters);
	wifiClient.print(String("GET ") + forecastIOurl + forecastIOkey + "/" + String(lat,7) + "," +String(lon,7) + forecastIOparameters + " HTTP/1.1\r\n" +
		"Host: " + forecastIOhost + "\r\n" +
		"User-Agent: ESP8266\r\n" +
		"Connection: close\r\n\r\n");
	Serial.println("request sent");
	while (wifiClient.connected()) {
		response += wifiClient.readString();
	}

	Serial.println("Got response:");
	if (response != "") {
		Serial.println(response);
		int index = response.indexOf("\"temperature\":");
		if (index) {
			String tempStr = response.substring(index);
			//Serial.println(tempStr);
			tempStr = tempStr.substring(tempStr.indexOf(":")+1, tempStr.indexOf(","));
			//Serial.println(tempStr);
			temperature = tempStr.toFloat();
			Serial.println("\nTemp: " + String(temperature));
		}
		index = response.indexOf("\"apparentTemperature\":");
		if (index) {
			String tempStr = response.substring(index);
			//Serial.println(tempStr);
			tempStr = tempStr.substring(tempStr.indexOf(":") + 1, tempStr.indexOf(","));
			//Serial.println(tempStr);
			apparentTemp = tempStr.toFloat();
			Serial.println("\nApparent Temp: " + String(apparentTemp) + "\n");
		}
	}

	//Serial.println("disconnected: " + String(wifiClient.connected()));

}


/* void temperatureFrame(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
	String temp = String (temperature)+"ºC";
	
	display->setTextAlignment(TEXT_ALIGN_CENTER);
	display->setFont(ArialMT_Plain_24);
	display->drawString(clockCenterX + x, clockCenterY + y, temp);
	temp = "a"+String(apparentTemp) + "ºC";
	display->drawString(clockCenterX + x, 16 + y, temp);
}*/

// Displays clock and temperature in OLED
void tempClockFrame(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
	String timenow = String(hour()) + ":" + twoDigits(minute()) + ":" + twoDigits(second());
	//String datenow = twoDigits(day()) + "/" + twoDigits(month()) + "/" + String(year());
	display->setTextAlignment(TEXT_ALIGN_CENTER);
	display->setFont(ArialMT_Plain_24);
	display->drawString(clockCenterX + x, clockCenterY + y, timenow);
	display->setFont(ArialMT_Plain_10);
	display->drawString(clockCenterX + x, 16 + y, String(temperature)+"ºC");
	display->drawString(clockCenterX + x, 30 + y, String(apparentTemp) + "ºC");
}


// This array keeps function pointers to all frames
// frames are the single views that slide in
FrameCallback frames[] = { tempClockFrame };

// how many frames are there?
int frameCount = 1;

// Overlays are statically drawn on top of a frame eg. a clock
OverlayCallback overlays[] = { clockOverlay };
int overlaysCount = 1;
Ticker tkTemp;

#ifndef SECRET
char[] ssid = "YOUR SSID";
char[] passwd = "YOUR WIFI PASSWORD";
#endif

void setup() {
  Serial.begin(115200);
  Serial.println();
  WiFi.begin(ssid, passwd);
  
  while (!WiFi.isConnected()) {
	  Serial.print(".");
	  delay(100);
  }
  NTP.begin(); // Starts NTP client
  NTP.setDayLight(true);
  NTP.setTimeZone(1); // TimeZone CET
  Serial.println();
	// The ESP is capable of rendering 60fps in 80Mhz mode
	// but that won't give you much time for anything else
	// run it in 160Mhz mode or just set it to 30 fps
  ui.setTargetFPS(30);

	// Customize the active and inactive symbol
  ui.setActiveSymbol(activeSymbol);
  ui.setInactiveSymbol(inactiveSymbol);
  ui.disableAutoTransition();

  // You can change this to
  // TOP, LEFT, BOTTOM, RIGHT
  ui.setIndicatorPosition(TOP);
  ui.disableIndicator();
  ui.disableAllIndicators();

  // Defines where the first frame is located in the bar.
  //ui.setIndicatorDirection(LEFT_RIGHT);

  // You can change the transition that is used
  // SLIDE_LEFT, SLIDE_RIGHT, SLIDE_UP, SLIDE_DOWN
  //ui.setFrameAnimation(SLIDE_LEFT);

  // Add frames
  ui.setFrames(frames, frameCount);

  // Add overlays
  ui.setOverlays(overlays, overlaysCount);

  // Initialising the UI will init the display too.
  ui.init();

  display.flipScreenVertically();

  //unsigned long secsSinceStart = millis();
  // Unix time starts on Jan 1 1970. In seconds, that's 2208988800:
  //const unsigned long seventyYears = 2208988800UL;
  // subtract seventy years:
  //unsigned long epoch = secsSinceStart - seventyYears * SECS_PER_HOUR;
  //setTime(epoch);
  getGeoFromWiFi();
  getTemperature();
}

void loop() {
  static unsigned long lastTime = 0;
  int remainingTimeBudget = ui.update();

  if (remainingTimeBudget > 0) {
    // You can do some work here
    // Don't do stuff if you are below your
    // time budget.
	  if ((millis() - lastTime) > (1000 * 300)) { // Every 5 minutes get temerature info
		  getTemperature(); 
		  lastTime = millis();
	  }
    delay(remainingTimeBudget);

  }


}
