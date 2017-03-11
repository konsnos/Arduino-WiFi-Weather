# Weather wemos #

Gets the weather info from openweathermap by sending a get request.
Parses the response and stores all usefuil info into variables.
This is done by taking into acount 3 forecasts of the next 3 hour timespans.
Calculates the info to show to the LCD display.
The display swaps screens and loops to show the info.

The circuit:

* A Liquid Crystal Dislay similar to the one referenced here https://www.arduino.cc/en/Tutorial/LiquidCrystalDisplay

* I used a Wemos D1 R2 as a board with the ESP-8266EX microcotroller, 
  but the code will probably work with anything having a WiFi module. ( https://www.wemos.cc/product/d1.html )

Video showcase: https://youtu.be/3M31VxNbEMs

Author: Konstantinos Egkarchos

Created at 10/03/2017

Project home: https://bitbucket.org/konsnos/arduino_wifi_weather