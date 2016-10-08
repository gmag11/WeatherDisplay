# WeatherDisplay
Weather information display without sensors using ESP8266 and OLED display

##Introduction
This project is a test to use my new SSD1306 OLED display. It is basically asome kind of fake weather station with clock. I say fake because it does not use any sensor connected to ESP8266 but gets information from [forecast.io](http://forecast.io), an API that allows to get weather information from national and international weather observation networks.
First, I hardcoded my location geographical coordinates, but, as I was playing with Internet APIs I tried to find how could I get my location without connecting a GPS. I noticed that Google offers a way to get location by sending surrounding WiFi, [Google Geolocation API](https://developers.google.com/maps/documentation/geolocation/intro).
Using this Geolocation and NTP syncronization, it is only needed to add WiFi information (SSID and password) and your Google and Forecast.io secret API keys to get it working.
##Dependances
SSD1306 OLED library: https://github.com/squix78/esp8266-oled-ssd1306
NTPClient library: https://github.com/gmag11/NtpClient
##Connection
OLED display is controlled using I2C protocol. Pins used on ESP8266 are 