# Weather wemos #

Gets the weather info from openweathermap by sending a get request.
Parses the response and stores all usefuil info into variables.
This is done by taking into acount 3 forecasts of the next 3 hour timespans.
Calculates the info to show to the LCD display.
The display swaps screens and loops to show the info.

## The circuit ##

* A Liquid Crystal Dislay similar to the one referenced here https://www.arduino.cc/en/Tutorial/LiquidCrystalDisplay

* I used a Wemos D1 R2 as a board with the ESP-8266EX microcotroller, 
  but the code will probably work with anything having a WiFi module. ( https://www.wemos.cc/product/d1.html )

Video showcase: https://youtu.be/3M31VxNbEMs

## Set Up ##
After running for the first time the project will not find any WiFi to log in to and it will create a server without a password at which a client may be connected. The local ip will be printed in the serial monitor if it exists.

The server page will ask for credentials for wifi and geographical info plus the openweathermap api to parse weather info.

AFter the project will connect to the WiFi the local ip will again be printed in the serial monitor if exists, and any client may hit this page to update the stored info.

# Credits #
Author: Konstantinos Egkarchos

Created at 10/03/2017

Project home: https://bitbucket.org/konsnos/arduino_wifi_weather