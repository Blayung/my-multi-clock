//https://wttr.in/?m&lang=en&format=%25C%5CnTemperature%3A+%25t%5CnWind%3A+%25w%5CnPressure%3A+%25P
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266HTTPClient.h>
#include <LiquidCrystal_I2C.h>
#include <ArduinoJson.h>
#include <time.h>

//COMPILER CONSTANTS
#define D0 16  //idk why aren't they defined by default

//WIFI CREDENTIALS
const String WIFI_SSID = "wifi_name";
const String WIFI_PASSWORD = "wifi_password";

//wifi & http clients
WiFiClient wifiClient;
HTTPClient httpClient;

// LCD Screen
LiquidCrystal_I2C lcd(0x27, 20, 4);

//Normal vars
String tempStr;
short httpResponseCode;
StaticJsonDocument<1024> jsonDocument;
DeserializationError jsonDeserializationError;
unsigned long startMillisTime;
unsigned long beforeMillisTime = 0;
time_t startTimestamp;
time_t nowTimestamp;
bool isButtonPressed;
bool wasButtonPressed = false;
uint8_t currentScreen = 0;
char formattedTimeUpperRow[8];
char formattedTimeBottomRow[10];

void error(String msg){ // Error handling
  Serial.println("Error message:");
  Serial.println(msg);
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.noBlink();
  lcd.noCursor();
  lcd.print("Error message:");
  lcd.setCursor(0,1);
  lcd.print(msg);
  while(true){delay(1);}
}

void updateTime() {  //Update the startTimestamp and startMillisTime variables
  httpClient.begin(wifiClient, "http://www.worldtimeapi.org:80/api/ip");

  httpResponseCode = httpClient.GET();

  if (httpResponseCode > 199 && httpResponseCode < 300) {
    jsonDeserializationError = deserializeJson(jsonDocument, httpClient.getString());
    if (jsonDeserializationError) {
      tempStr="Parsing worldtimeapi json failed: ";
      tempStr+=jsonDeserializationError.c_str();
      error(tempStr);
    } else if (jsonDocument.containsKey("unixtime")) {
      startTimestamp = jsonDocument["unixtime"];
      startMillisTime = millis();
    } else {
      error("Couldn't get the unixtime key from worldtimeapi json data!");
    }
  } else {
    tempStr="Recieved an invalid response code from worldtimeapi: ";
    tempStr+=httpResponseCode;
    error(tempStr);
  }

  httpClient.end();
}

void setup() {
  //Serial begin
  Serial.begin(74880);
  Serial.println();
  Serial.println("Starting...");

  //LCD init
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.noBlink();
  lcd.noCursor();

  // The loading screen
  lcd.setCursor(3,0);
  lcd.print("Welcome to the");
  lcd.setCursor(2,1);
  lcd.print("multi-clock 5000!");
  lcd.setCursor(0,2);
  lcd.print("====================");
  lcd.setCursor(5,3);
  lcd.print("Loading...");

  //SETUP PINS
  pinMode(D0, INPUT);

  // Connect to wifi
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  startMillisTime = millis();
  while (WiFi.status() != WL_CONNECTED) {
    if (millis() - startMillisTime > 10000) {
      tempStr="The connection to wifi timed out! Wifi status code: ";
      tempStr+=WiFi.status();
      error(tempStr);
    }
    delay(1);
  }

  //Update the time and set the timezone
  setenv("TZ", "CET-2", 1);  // The middle argument is the timezone, and it's in a very weird-ass format, but you basically need to write it like that: <THREE LETTER CODE><A MINUS IF YOU WANT IT TO BE ON UTC+ OR NOTHING IF YOU WANT IT TO BE ON UTC-><OFFSET FROM UTC>
  tzset();
  updateTime();

  //End the loading screen
  lcd.clear();
  lcd.setCursor(0,0);
}

void loop() {
  if (beforeMillisTime > millis()) {  // Check for millis() overflowing
    updateTime();
  }
  beforeMillisTime = millis();

  //Input handling
  if (digitalRead(D0) == LOW) {
    isButtonPressed = true;
  } else {
    isButtonPressed = false;
  }
  if (isButtonPressed && !wasButtonPressed) {
    currentScreen++;
    if(currentScreen==5){
      currentScreen=0;
    }
    lcd.clear();
  }
  wasButtonPressed = isButtonPressed;

  // THE SCREENS
  if (currentScreen == 0) {  // The clock screen
    nowTimestamp = startTimestamp + ((millis() - startMillisTime) / 1000);
    strftime(&formattedTimeUpperRow[0], 9, "%H:%M:%S", localtime(&nowTimestamp));
    lcd.setCursor(6,1);
    lcd.print(formattedTimeUpperRow);
    strftime(&formattedTimeBottomRow[0], 11, "%d/%m/%Y", localtime(&nowTimestamp));
    lcd.setCursor(5,2);
    lcd.print(formattedTimeBottomRow);
  } else if (currentScreen == 1) {  // The weather screen
    lcd.setCursor(0,0);
    lcd.print("WEATHER INFO");
  } else if (currentScreen == 2) {  // The car game screen
    lcd.setCursor(0,0);
    lcd.print("BRRR BRRR CAR GO FAST");
  } else if (currentScreen == 3) {  // The minesweeper screen
    lcd.setCursor(0,0);
    lcd.print("MINESWEEEEEPIE");
  } else if (currentScreen == 4) {  // Days until vacation screen
    lcd.setCursor(0,0);
    lcd.print("INF DAYS TIL VACATION");
  }
}
