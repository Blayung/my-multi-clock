#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266HTTPClient.h>
#include <LiquidCrystal_I2C.h>
#include <ArduinoJson.h>
#include <time.h>

// Compiler constants
#define D0 16 //idk why isn't it defined by default

// Wifi credentials
const String WIFI_SSID = "wifi_name";
const String WIFI_PASSWORD = "wifi_password";

// Wifi & http clients
WiFiClient wifiClient;
HTTPClient httpClient;

// The LCD Screen
LiquidCrystal_I2C lcd(0x27, 20, 4);

// Other variables
unsigned long startMillisTime;
unsigned long beforeMillisTime = 0;
time_t startTimestamp;
time_t nowTimestamp;
tm* now;
bool isButtonPressed;
bool wasButtonPressed = false;
uint8_t currentScreen = 0;
char clockScreenSecondRow[8];
char clockScreenThirdRow[10];
char clockScreenFourthRow[20];

void error(String msg) { // Error handling
    Serial.println("Error message:");
    Serial.println(msg);

    lcd.clear();
    lcd.setCursor(0,0);
    lcd.noBlink();
    lcd.noCursor();
    lcd.print("Error message:");
    lcd.setCursor(0,1);
    lcd.print(msg);

    while (true) {
        delay(1);
    }
}

void updateTime() { // Update the startTimestamp and startMillisTime variables
    httpClient.begin(wifiClient, "http://www.worldtimeapi.org:80/api/ip");

    short httpResponseCode = httpClient.GET();

    if (httpResponseCode > 199 && httpResponseCode < 300) {
        StaticJsonDocument<1024> jsonDocument;
        DeserializationError jsonDeserializationError = deserializeJson(jsonDocument, httpClient.getString());
        if (jsonDeserializationError) {
            String tempStr = "Parsing worldtimeapi json failed: ";
            tempStr += jsonDeserializationError.c_str();
            error(tempStr);
        } else if (jsonDocument.containsKey("unixtime")) {
            startTimestamp = jsonDocument["unixtime"];
            startMillisTime = millis();
        } else {
            error("Couldn't get the unixtime key from worldtimeapi json data!");
        }
    } else {
        String tempStr = "Recieved an invalid response code from worldtimeapi: ";
        tempStr += httpResponseCode;
        error(tempStr);
    }

    httpClient.end();
}

void setup() {
    // Serial begin
    Serial.begin(74880);
    Serial.println();
    Serial.println("Starting...");

    // LCD init
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

    // Setting up pins
    pinMode(D0, INPUT);

    // Connecting to wifi
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(100);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    startMillisTime = millis();
    while (WiFi.status() != WL_CONNECTED) {
        if (millis() - startMillisTime > 10000) {
            String tempStr = "The connection to wifi timed out! Wifi status code: ";
            tempStr += WiFi.status();
            error(tempStr);
        }
        delay(1);
    }

    // Setting the timezone and updating the time
    setenv("TZ", "CET-2", 1);  // The middle argument is the timezone, and it's in a very weird-ass format, but you basically need to write it like that: <RANDOM THREE LETTER CODE><A MINUS IF YOU WANT IT TO BE ON UTC+ OR NOTHING IF YOU WANT IT TO BE ON UTC-><OFFSET FROM UTC>
    tzset();
    updateTime();

    // Clearing the loading screen
    lcd.clear();
    lcd.setCursor(0,0);
}

void loop() {
    if (beforeMillisTime > millis()) { // Check for millis() overflowing
        updateTime();
    }
    beforeMillisTime = millis();

    // Input
    if (digitalRead(D0) == HIGH) {
        isButtonPressed = true;
    } else {
        isButtonPressed = false;
    }

    if (isButtonPressed && !wasButtonPressed) {
        currentScreen++;
        if (currentScreen == 3) {
            currentScreen = 0;
        }
        lcd.clear();
    }

    wasButtonPressed = isButtonPressed;

    // THE SCREENS
    if (currentScreen == 0) { // The clock screen
        nowTimestamp = startTimestamp + ((millis() - startMillisTime) / 1000);
        now = localtime(&nowTimestamp);

        //1st row
        //(days until vacation in "UNTIL VAC D:X H:X" format)

        //2nd row
        strftime(&clockScreenSecondRow[0], 9, "%H:%M:%S", now);
        lcd.setCursor(6,1);
        lcd.print(clockScreenSecondRow);

        //3rd row
        strftime(&clockScreenThirdRow[0], 11, "%d/%m/%Y", now);
        lcd.setCursor(5,2);
        lcd.print(clockScreenThirdRow);

        //4th row
        clockScreenFourthRow[0]=' ';clockScreenFourthRow[1]=' ';clockScreenFourthRow[2]=' ';clockScreenFourthRow[3]=' ';clockScreenFourthRow[4]=' ';clockScreenFourthRow[5]=' ';clockScreenFourthRow[6]=' ';clockScreenFourthRow[7]=' ';clockScreenFourthRow[8]=' ';clockScreenFourthRow[9]=' ';clockScreenFourthRow[10]=' ';clockScreenFourthRow[11]=' ';clockScreenFourthRow[12]=' ';clockScreenFourthRow[13]=' ';clockScreenFourthRow[14]=' ';clockScreenFourthRow[15]=' ';clockScreenFourthRow[16]=' ';clockScreenFourthRow[17]=' ';clockScreenFourthRow[18]=' ';clockScreenFourthRow[19]=' ';
        lcd.setCursor((20 - strftime(&clockScreenFourthRow[0], 21, "%A, %B", now)) / 2, 3);
        lcd.print(clockScreenFourthRow);
    } else if (currentScreen == 1) {  // The weather screen, https://wttr.in/?m&lang=en&format=%25C%5CnTemperature%3A+%25t%5CnWind%3A+%25w%5CnPressure%3A+%25P
        lcd.setCursor(0,0);
        lcd.print("WEATHER INFO");
    } else if (currentScreen == 2) {  // The car game screen
        lcd.setCursor(0,0);
        lcd.print("BRRR BRRR CAR GO FAST");
    }
}
