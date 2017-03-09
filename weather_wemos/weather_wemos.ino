#include <LiquidCrystal.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

// initialize the library with the numbers of the interface pins
LiquidCrystal lcd(D7, D6, D4, D3, D2, D1);
const int contrastPin = D5;

const int screenDelay = 3000;

/**** Weather variables according to openweatherdata ( http://openweathermap.org/forecast5 ) ****/
const int arrayLength = 3;
int tmpsMin[arrayLength];
int tmpsMax[arrayLength];
String descriptions[arrayLength];
float wndSpds[arrayLength];
float rainmm[arrayLength];
float snowmm[arrayLength];

int tmpMin;
int tmpMax;
float wndSpd;
float rainTotal;
float snowTotal;
/**** END WEATHER VARIABLES ****/

/**** WIFI VARIABLES ********/
const char* WIFI_SSID     = "";
const char* WIFI_PSWD = "";

const char* weatherForecastURL = "";
/***** END WIFI VARIABLES ******/

int screenCount;
int currentScreenDisplaying;

void setup() 
{
  // debug
  Serial.begin(9600);

  setupLCD();
  setupWiFi(WIFI_SSID, WIFI_PSWD);
  requestWeather(weatherForecastURL);

  calculateWeather();
  
  currentScreenDisplaying = -1; // -1 to show 0 first
  
  Serial.println("setup done");
}

void loop() 
{
  showScreen(getScreen());
    
  delay(screenDelay);
}

int getScreen()
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
        Serial.println("Unregistered screen display: " + (String)currentScreenDisplaying);
        return 0;
    }
  }
}

const int descriptionsInitialScreen = 5;
void showScreen(int currentScreen)
{
  Serial.println("Display: " + (String)currentScreen);
  
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
      Serial.println("Unregistered screen display: " + (String)currentScreen);
      break;
  }
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
  Serial.println("Calculating weather");
  
  // calculate minimum temperature
  tmpMin = tmpsMin[0];
  for(int i = 1;i<(sizeof(tmpsMin)/sizeof(int));i++)
  {
    if(tmpsMin[i] < tmpMin)
      tmpMin = tmpsMin[i];
  }
  tmpMin = floor(tmpMin);

  Serial.println("Calculated min temperature");

  // calculate maximum temperature
  tmpMax = tmpsMax[0];
  for(int i = 1;i<(sizeof(tmpsMax)/sizeof(int));i++)
  {
    if(tmpsMax[i] > tmpMax)
      tmpMax = tmpsMax[i];
  }
  tmpMax = ceil(tmpMax);

  Serial.println("Calculated max temperature");

  // find fastest wind
  wndSpd = wndSpds[0];
  for (int i = 1;i<(sizeof(wndSpds)/sizeof(float));i++)
  {
    if(wndSpds[i] > wndSpd)
      wndSpd = wndSpds[i];
  }

  Serial.println("Calculated wind speed");

  // find total rain
  // if totalRain equals to 0 it will be ignored
  rainTotal = rainmm[0];
  for(int i = 1;i<(sizeof(rainmm)/sizeof(float));i++)
  {
    rainTotal += rainmm[i];
  }

  // find total snow
  // if totalSnow equals to 0 it will be ignored
  snowTotal = snowmm[0];
  for(int i = 1;i<(sizeof(snowmm)/sizeof(float));i++)
  {
    snowTotal += snowmm[i];
  }

  screenCount = 5;
  // remove duplicate descriptions
  int count = arrayLength;
  int offset = 0;
  for(int i = 0;i<count;i++)
  {
    if(offset > 0)
    {
      descriptions[i-offset] = descriptions[i];
    }

    if(descriptions[i] == "")
    {
      offset++;
      continue;
    }
    
    for(int j = i + 1;j<count;j++)
    {
      if(descriptions[j] != "" && descriptions[i] == descriptions[j])
      {
        descriptions[j] = "";
      }
    }
  }
  screenCount += (count - offset);

  Serial.println("Calculated descriptions");
}

void requestWeather(const char* requestURL)
{
  if(WiFi.status() == WL_CONNECTED)
  {
    // debug
    Serial.print("Requesting ");
    Serial.println(requestURL);
    
    HTTPClient http;

    http.begin(requestURL);
    int httpCode = http.GET();

    if(httpCode > 0)
    {
      setupWeather(http.getString());
    }
    else
    {
      Serial.println((String)"Request returned code: "+ httpCode);
      lcd.clear();
      lcd.print((String)"Error: " + httpCode);
      delay(100000);
    }
    http.end();
  }
}

void setupWiFi(const char* ssid, const char* password)
{
  /* Explicitly set the ESP8266 to be a WiFi-client, otherwise, it by default,
   would try to act as both a client and an access-point and could cause
   network-issues with your other WiFi-devices on your WiFi-network. */
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  lcd.clear();
  lcd.print("Connecting");
  lcd.setCursor(0, 1);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    lcd.print(".");
  }

  // debug
  Serial.println("");
  Serial.println("WiFi connected");  
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
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

    Serial.println((String)"Test tmpsMin: "+ tmpsMin[index]);
  }

  // get max temperature
  int tmpMaxIndex = weatherResponse.indexOf("temp_max", strStart);
  if(tmpMaxIndex > 0 && tmpMaxIndex < strEnd)
  {
    nextComma = weatherResponse.indexOf(",", tmpMaxIndex);
    tmpsMax[index] = ceil(weatherResponse.substring(tmpMaxIndex+10, nextComma).toFloat());

    Serial.println((String)"Test tmpsMax: "+ tmpsMax[index]);
  }

  // get descriptions
  int tmpDescription = weatherResponse.indexOf("description", strStart);
  if(tmpDescription > 0 && tmpDescription < strEnd)
  {
    nextComma = weatherResponse.indexOf(",", tmpDescription);
    descriptions[index] = weatherResponse.substring(tmpDescription+14, nextComma-1);

    Serial.println((String)"Test description: " + descriptions[index]);
  }
  else
  {
    Serial.println((String)"Description wasn't found " + tmpDescription);
  }

  // get wind speed
  int tmpWndSpd = weatherResponse.indexOf("speed", strStart);
  if(tmpWndSpd > 0 && tmpWndSpd < strEnd)
  {
    nextComma = weatherResponse.indexOf(",", tmpWndSpd);
    wndSpds[index] = weatherResponse.substring(tmpWndSpd+7, nextComma).toFloat();

    Serial.println((String)"Test wind speed: " + wndSpds[index]);
  }
  else 
  {
    Serial.println((String)"Wind speed wasn't found " + tmpWndSpd);
  }

  // get rainmm
  int tmpRainmm = weatherResponse.indexOf(',"rain":', strStart);
  if(tmpRainmm > 0 && tmpRainmm < strEnd) // valid rain
  {
    int tmpRain3h = weatherResponse.indexOf("3h", tmpRainmm);
    if(tmpRain3h > -1 && tmpRain3h < strEnd)  // 3h must exist
    {
      nextComma = weatherResponse.indexOf("}"); // value has object ending.
      rainmm[index] = weatherResponse.substring(tmpRain3h+4, nextComma).toFloat();

      Serial.println((String)"Rainmm index: " + tmpRainmm);
      Serial.println((String)"Rainmm 3h index: " + tmpRain3h);
      Serial.println("Test rain string: " + weatherResponse.substring(tmpRain3h+4, nextComma));
      Serial.println((String)"Test rainmm: " + rainmm[index]);
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
  /*int tmpSnowmm = weatherResponse.indexOf("snow", strStart);
  if(tmpSnowmm > 0 && tmpSnowmm < strEnd) // valid rain
  {
    int tmpSnow3h = weatherResponse.indexOf("3h", tmpSnowmm);
    nextComma = weatherResponse.indexOf("}"); // value has object ending.
    snowmm[index] = weatherResponse.substring(tmpSnow3h+4, nextComma).toFloat();

    Serial.println((String)"Test snowmm: " + snowmm[index]);
  }
  else
  {
    snowmm[index] = 0;
  }*/
}

void setupWeather(String weatherResponse)
{
  Serial.println("Setting up weather");

  // FIRST VALUES
  char* indexWord = "temp_min";
  int indexWordLength = sizeof(indexWord);
  
  int index1 = weatherResponse.indexOf(indexWord);
  int index2 = weatherResponse.indexOf(indexWord, index1 + indexWordLength);

  storeWeather(weatherResponse, 0, index1, index2);
  
  tmpsMin[1] = 10;
  tmpsMin[2] = 13;
  
  tmpsMax[1] = 13;
  tmpsMax[2] = 16;
  
  descriptions[1] = "scattered clouds";
  descriptions[2] = "clear sky";
  
  wndSpds[1] = 7.41;
  wndSpds[2] = 3.5;

  rainmm[1] = 0.37;
  rainmm[2] = 0.00;

  snowmm[1] = 0.02;
  snowmm[2] = 0.05;
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

