/* 
 *  Weather wemos
 *  
 *  Gets the weather info from openweathermap by sending a get request.
 *  Parses the response and stores all usefuil info into variables.
 *  This is done by taking into acount 3 forecasts of the next 3 hour timespans.
 *  Calculates the info to show to the LCD display.
 *  The display swaps screens and loops to show the info.
 *  
 *  The circuit:
 *  * A Liquid Crystal Dislay similar to the one referenced here https://www.arduino.cc/en/Tutorial/LiquidCrystalDisplay
 *  * I used a Wemos D1 R2 as a board with the ESP-8266EX microcotroller, 
 *    but the code will probably the code will work with anything having a WiFi module. ( https://www.wemos.cc/product/d1.html )
 *  
 *  Author: Konstantinos Egkarchos
 *  Created at 10/03/2017
 *  Project home: https://bitbucket.org/konsnos/arduino_wifi_weather
 */
#include <LiquidCrystal.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>

// initialize the library with the numbers of the interface pins
LiquidCrystal lcd(D7, D6, D4, D3, D2, D1);
const byte contrastPin = D5;

const short screenDelay = 3000;

const int EEPROM_SIZE = 512;

/**** Weather variables according to openweathermap ( http://openweathermap.org/forecast5 ) ****/
// if samplesLength changes, also change the cnt variable in the request.
const byte samplesLength = 3;
short tmpsMin[samplesLength];
short tmpsMax[samplesLength];
char descriptions[samplesLength][17];
float wndSpds[samplesLength];
float rainmm[samplesLength];
float snowmm[samplesLength];

short tmpMin;
short tmpMax;
float wndSpd;
float rainTotal;
float snowTotal;
/**** END WEATHER VARIABLES ****/

ESP8266WebServer server(80);
boolean serverInitialised;
// True when connectted to wifi. If false then it's only server.
boolean connectedToWiFi;
const char* WIFI_SERVER_NAME = "Weather_For";
boolean showingServerScreen;

// A sample openweathermap.org request url
// It is useful to add the cnt=3 variable to the url to make the string returned, shorter.
// Remember to change the cnt variable according to the variable samplesLength.
const char* weatherForecastURL = "http://api.openweathermap.org/data/2.5/forecast?lat=";
const char* weatherForLon = "&lon=";
const char* weatherForAppId = "&units=metric&cnt=3&appid=";
/***** END WIFI VARIABLES ******/

byte screenCount;
byte currentScreenDisplaying;

void setup()
{
  // debug
  connectedToWiFi = false;
  serverInitialised = false;
  showingServerScreen = false;
  Serial.begin(9600);

  EEPROM.begin(EEPROM_SIZE);

  setupLCD();
  setupWiFi();
  requestWeather();

  calculateWeather();
  
  currentScreenDisplaying = -1; // -1 to show 0 first
}

void loop() 
{
  // handles both access point and client servers.
  server.handleClient();
  
  if(connectedToWiFi)
  {
    showingServerScreen = false;
    
    showScreen(getScreen());
    
    delay(screenDelay);
  }
  else
  {
    if(!showingServerScreen)
      showServerScreen();
    
    showingServerScreen = true;
  }
}

byte getScreen()
{
  for(;;) // infinite loop until a screen is found
  {
    currentScreenDisplaying++;
    
    if(currentScreenDisplaying >= screenCount)
      currentScreenDisplaying = 0;
    
    switch(currentScreenDisplaying)
    {
      case 0: // title
      case 1: // temperature
      case 2: // wind speed
      case 5: // description
      case 6: // description
      case 7: // description
        return currentScreenDisplaying;
      case 3: // rain
        if(rainTotal > 0) // if there is rain show it. else ignore it
          return currentScreenDisplaying;
        break;
      case 4:
        if(snowTotal > 0) // if there is rain show it. else ignore it
          return currentScreenDisplaying;
        break;
      default:
        Serial.println("current Screen Displaying error");
        return 0;
    }
  }
}

const byte descriptionsInitialScreen = 5;
void showScreen(byte currentScreen)
{
  //Serial.println("Display: " + (String)currentScreen);
  
  switch(currentScreen)
  {
    case 0:
      showTitle();
      break;
    case 1:
      showTemperature();
      break;
    case 2:
      showWindSpeed();
      break;
    case 3:
      showTotalRainVolume();
      break;
    case 4:
      showTotalSnowVolume();
      break;
    case 5:
    case 6:
    case 7:
      showDescription(currentScreen);
      break;
    default:
      Serial.println("currentScreen error");
      break;
  }
}

/// Prints server name and IP
void showServerScreen()
{
  lcd.clear();
  lcd.print(WIFI_SERVER_NAME);
  lcd.setCursor(0, 1);
  lcd.print(WiFi.softAPIP());
}

void showTitle()
{
  lcd.clear();
  lcd.print("Weather");
  lcd.setCursor(0, 1);
  lcd.print("Forecast");
}

void showTemperature()
{
  lcd.clear();
  lcd.print("Temperature");
  lcd.setCursor(0, 1);
  lcd.print(tmpMin + (String)"-" + tmpMax + " C");
}

void showWindSpeed()
{
  lcd.clear();
  lcd.print("Wind Speed");
  lcd.setCursor(0, 1);
  lcd.print("Max: " + (String)wndSpd + " mps");
}

void showTotalRainVolume()
{
  lcd.clear();
  lcd.print("Rain Volume");
  lcd.setCursor(0, 1);
  lcd.print((String)rainTotal + " mm");
}

void showTotalSnowVolume()
{
  lcd.clear();
  lcd.print("Snow Volume");
  lcd.setCursor(0, 1);
  lcd.print((String)snowTotal + " mm");
}

void showDescription(int currentScreen)
{
  lcd.clear();
  lcd.print("Description");
  lcd.setCursor(0, 1);
  lcd.print(descriptions[currentScreen - descriptionsInitialScreen]);
}

/// Calculates weather variables to show.
/// Calculates screens for the display.
void calculateWeather()
{
  // calculate minimum temperature
  tmpMin = tmpsMin[0];
  for(byte i = 1;i<samplesLength;i++)
  {
    if(tmpsMin[i] < tmpMin)
      tmpMin = tmpsMin[i];
  }
  tmpMin = floor(tmpMin);

  // calculate maximum temperature
  tmpMax = tmpsMax[0];
  for(byte i = 1;i<samplesLength;i++)
  {
    if(tmpsMax[i] > tmpMax)
      tmpMax = tmpsMax[i];
  }
  tmpMax = ceil(tmpMax);

  // find fastest wind
  wndSpd = wndSpds[0];
  for (byte i = 1;i<samplesLength;i++)
  {
    if(wndSpds[i] > wndSpd)
      wndSpd = wndSpds[i];
  }

  // find total rain
  // if totalRain equals to 0 it will be ignored
  rainTotal = rainmm[0];
  for(byte i = 1;i<samplesLength;i++)
  {
    rainTotal += rainmm[i];
  }

  // find total snow
  // if totalSnow equals to 0 it will be ignored
  snowTotal = snowmm[0];
  for(byte i = 1;i<samplesLength;i++)
  {
    snowTotal += snowmm[i];
  }

  screenCount = 5;
  // remove duplicate descriptions
  byte offset = 0;
  for(byte i = 0;i<samplesLength;i++)
  {
    if(offset > 0)
    {
      strcpy(descriptions[i-offset], descriptions[i]);
    }

    if(descriptions[i][0] == (char)0)
    {
      offset++;
      continue;
    }
    
    for(byte j = i + 1;j<samplesLength;j++)
    {
      if(descriptions[j][0] != (char)0 && strcmp(descriptions[i], descriptions[j]) == 0)
      {
        descriptions[j][0] = (char)0;
      }
    }
  }
  screenCount += (samplesLength - offset);
}

void requestWeather()
{
  if(WiFi.status() == WL_CONNECTED)
  {
    HTTPClient http;

    // create the request url
    int addr = 0;
    readCharFromEEPROM(&addr);
    readCharFromEEPROM(&addr);
    char * latitude = readCharFromEEPROM(&addr);
    char * longitude = readCharFromEEPROM(&addr);
    char * apiKey = readCharFromEEPROM(&addr);

    String requestURL = (String)weatherForecastURL + latitude + weatherForLon + longitude + weatherForAppId + apiKey;
    
    http.begin(requestURL);
    short httpCode = http.GET();

    if(httpCode > 0)
    {
      setupWeather(http.getString());
    }
    else
    {
      Serial.println("request error");
      lcd.clear();
      lcd.print((String)"Error: " + httpCode);
      delay(100000);
    }
    http.end();
  }
}

void setupWiFi()
{
  Serial.println("wifi");
  
  showingServerScreen = false;
  int addr = 0;
  char * WIFI_SSID = readCharFromEEPROM(&addr);
  char * WIFI_PSWD = readCharFromEEPROM(&addr);

  /* Explicitly set the ESP8266 to be a WiFi-client, otherwise, it by default,
   would try to act as both a client and an access-point and could cause
   network-issues with your other WiFi-devices on your WiFi-network. */
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PSWD);
  lcd.clear();
  lcd.print("Connecting");
  lcd.setCursor(0, 1);

  byte countDelays = 0;
  while (WiFi.status() != WL_CONNECTED && countDelays < 10)  // wait for 5 seconds
  {
    countDelays++;
    delay(500);
    lcd.print(".");
  }

  free(WIFI_SSID);
  free(WIFI_PSWD);

  if(WiFi.status() != WL_CONNECTED)
  {
    // initiate server
    WiFi.softAP(WIFI_SERVER_NAME);
    Serial.println("");
    Serial.print("Server addr: ");
    Serial.println(WiFi.softAPIP());
    connectedToWiFi = false;
  }
  else
  {
    Serial.println("");
    Serial.print("Local addr: ");
    Serial.println(WiFi.localIP());
    connectedToWiFi = true;
  }

  if(!serverInitialised)
  {
    Serial.println("initialising server");
    server.on("/", handleRoot);
    server.on("/saveData", handleData);
    server.onNotFound(handleNotFound);
    server.begin();
    serverInitialised = true;
  }
}

void storeWeather(String weatherResponse, int index, int strStart, int strEnd)
{
  int nextComma;

  // get min temperature
  int tmpMinIndex = weatherResponse.indexOf("temp_min", strStart);
  if(tmpMinIndex > 0 && tmpMinIndex < strEnd)  // is valid tmp min
  {
    nextComma = weatherResponse.indexOf(",", tmpMinIndex);
    tmpsMin[index] = floor(weatherResponse.substring(tmpMinIndex+10, nextComma).toFloat());
  }
  else
  {
    Serial.println("min tmp error");
  }

  // get max temperature
  int tmpMaxIndex = weatherResponse.indexOf("temp_max", strStart);
  if(tmpMaxIndex > 0 && tmpMaxIndex < strEnd)
  {
    nextComma = weatherResponse.indexOf(",", tmpMaxIndex);
    tmpsMax[index] = ceil(weatherResponse.substring(tmpMaxIndex+10, nextComma).toFloat());
  }
  else
  {
    Serial.println("max tmp error");
  }

  // get descriptions
  int tmpDescription = weatherResponse.indexOf("description", strStart);
  if(tmpDescription > 0 && tmpDescription < strEnd)
  {
    nextComma = weatherResponse.indexOf(",", tmpDescription);
    strcpy(descriptions[index], weatherResponse.substring(tmpDescription+14, nextComma-1).c_str());
  }
  else
  {
    Serial.println("description error");
  }

  // get wind speed
  int tmpWndSpd = weatherResponse.indexOf("speed", strStart);
  if(tmpWndSpd > 0 && tmpWndSpd < strEnd)
  {
    nextComma = weatherResponse.indexOf(",", tmpWndSpd);
    wndSpds[index] = weatherResponse.substring(tmpWndSpd+7, nextComma).toFloat();
  }
  else 
  {
    Serial.println("wind speed error");
  }

  // get rainmm
  int tmpRainmm = weatherResponse.indexOf("rain\":", strStart);
  if(tmpRainmm > 0 && tmpRainmm < strEnd) // valid rain
  {
    int tmpRain3h = weatherResponse.indexOf("3h", tmpRainmm);
    if(tmpRain3h > -1 && tmpRain3h < strEnd)  // 3h must exist
    {
      nextComma = weatherResponse.indexOf("}", tmpRain3h); // value has object ending.
      rainmm[index] = weatherResponse.substring(tmpRain3h+4, nextComma).toFloat();
    }
    else
    {
      rainmm[index] = 0;
    }
  }
  else
  {
    rainmm[index] = 0;
  }

  // get snowmm
  int tmpSnowmm = weatherResponse.indexOf("snow\":", strStart);
  if(tmpSnowmm > 0 && tmpSnowmm < strEnd) // valid rain
  {
    int tmpSnow3h = weatherResponse.indexOf("3h", tmpSnowmm);
    if(tmpSnow3h > -1 && tmpSnow3h < strEnd)  // 3h must exist
    {
      nextComma = weatherResponse.indexOf("}", tmpSnow3h); // value has object ending.
      snowmm[index] = weatherResponse.substring(tmpSnow3h+4, nextComma).toFloat();
    }
    else
    {
      snowmm[index] = 0;
    }
  }
  else
  {
    snowmm[index] = 0;
  }
}

void setupWeather(String weatherResponse)
{
  // FIRST VALUES
  const char* indexWord = "temp_min";
  byte indexWordLength = sizeof(indexWord);
  int index1;
  int index2;
  
  index1 = weatherResponse.indexOf(indexWord);
  index2 = weatherResponse.indexOf(indexWord, index1 + indexWordLength);
  storeWeather(weatherResponse, 0, index1, index2);
  // 2nd parse
  index1 = index2;
  index2 = weatherResponse.indexOf(indexWord, index1 + indexWordLength);
  storeWeather(weatherResponse, 1, index1, index2);
  // 3rd parse
  index1 = index2;
  index2 = weatherResponse.length();
  storeWeather(weatherResponse, 2, index1, index2);
}

void setupLCD()
{
  pinMode(contrastPin, OUTPUT);
  digitalWrite(contrastPin, LOW);
  
  // set up the LCD's number of columns and rows:
  lcd.begin(16, 2);
  // Print a message to the LCD.
  lcd.print("Initializing!");
}

// FUNCTIONS FOR SERVER
//root page can be accessed only if authentification is ok
void handleRoot()
{
  String msg = "Pass Credentials";
  String content = "<html><body><form action='/saveData' method='POST'>Provide credentials:<br>";
  content += "WiFi SSID:<input type='text' name='user' placeholder='user name'><br>";
  content += "WiFi Password:<input type='password' name='pass' placeholder='password'><br>";
  content += "Latitude:<input type='text' name='lat' placeholder='38.002047'><br>";
  content += "Longitude:<input type='text' name='longi' placeholder='23.723602'><br>";
  content += "Open WeatherMap API key:<input type='text' name='apiKey' placeholder='a651sdf65asd1f6a5sd1fg65df'><br>";
  content += "<input type='submit' name='SUBMIT' value='Submit'></form>" + msg + "<br>";
  server.send(200, "text/html", content);
}

/// Stores wifi username and password, latitude, longitude, and api key for openweathermap.
void handleData()
{
  if(server.hasArg("user") && server.hasArg("pass") && server.hasArg("lat") && server.hasArg("longi") && server.hasArg("apiKey"))
  {
    char* str;
    int addr = 0;
    
    server.arg("user").toCharArray(str, 40);
    writeToEEPROM(&addr, str);
    EEPROM.write(addr++, 0); // user written

    server.arg("pass").toCharArray(str, 40);
    writeToEEPROM(&addr, str);
    EEPROM.write(addr++, 0); // pass written

    server.arg("lat").toCharArray(str, 40);
    writeToEEPROM(&addr, str);
    EEPROM.write(addr++, 0); // pass latitude

    server.arg("longi").toCharArray(str, 40);
    writeToEEPROM(&addr, str);
    EEPROM.write(addr++, 0); // pass longitude
    
    server.arg("apiKey").toCharArray(str, 40);
    writeToEEPROM(&addr, str);
    EEPROM.write(addr++, 0); // pass longitude

    EEPROM.commit();

    setupWiFi();
  }
}

void handleNotFound()
{
  String message = "<html><body><H2>Page Not Found</H2><br><br>";
  message += "Back to <a href='/'>root</a>.<br><br></body></html>";
  server.send(404, "text/html", message);
}
// END FUNCTIONS FOR SERVER

// EEPROM FUNCTIONS


// writes variable to the permanent memory of the EEPROM
void writeToEEPROM(int * addr, char* str)
{
  for(byte i = 0;i<strlen(str);i++)
  {
    EEPROM.write((*addr)++, str[i]);
  }
}

// reads value until it finds 0 or 255 as value
char* readCharFromEEPROM(int * addr)
{
  char * str = (char *) malloc(40);
  byte i = 0;
  for (;i<40 && (*addr) < EEPROM_SIZE;i++)
  {
    byte value = EEPROM.read((*addr)++);
    if(value == 0 || value == 255)
      break;

    str[i] = char(value);
  }
  str[i] = 0;
  
  return str;
}
// END EEPROM FUNCTIONS
