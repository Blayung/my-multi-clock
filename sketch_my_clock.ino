#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266HTTPClient.h>
#include <LiquidCrystal_I2C.h>
#include <ArduinoJson.h>
#include <time.h>

#define D0 16

// WIFI CREDENTIALS
const String WIFI_SSID = "wifi name";
const String WIFI_PASSWORD = "wifi password";

WiFiClient wifiClient;
HTTPClient httpClient;

LiquidCrystal_I2C lcd(0x27, 20, 4);

void error(String msg) {
    Serial.println("Error:\n" + msg);

    lcd.clear();
    lcd.setCursor(0,0);
    lcd.noBlink();
    lcd.noCursor();
    lcd.print("Error:");
    lcd.setCursor(0,1);
    lcd.print(msg);

    while (true) {
        delay(1);
    }
}

unsigned long startMillisTime;
time_t startTimestamp;

void updateTime() {
    httpClient.begin(wifiClient, "http://www.worldtimeapi.org:80/api/ip");

    short httpResponseCode = httpClient.GET();

    if (httpResponseCode > 199 && httpResponseCode < 300) {
        JsonDocument jsonDocument;
        DeserializationError jsonDeserializationError = deserializeJson(jsonDocument, httpClient.getString());
        if (jsonDeserializationError) {
            String temp = "Parsing worldtimeapi json failed: ";
            temp += jsonDeserializationError.c_str();
            error(temp);
        } else if (jsonDocument.containsKey("unixtime")) {
            startTimestamp = jsonDocument["unixtime"];
            startMillisTime = millis();
        } else {
            error("Couldn't get the unixtime key from worldtimeapi json data!");
        }
    } else {
        String temp = "Recieved an invalid response code from worldtimeapi: ";
        temp += httpResponseCode;
        error(temp);
    }

    httpClient.end();
}

void setup() {
    Serial.begin(9600);
    Serial.println("\nStarted!");

    lcd.init();
    lcd.backlight();
    lcd.clear();
    lcd.noBlink();
    lcd.noCursor();

    lcd.setCursor(3,0);
    lcd.print("Welcome to the");
    lcd.setCursor(2,1);
    lcd.print("multi-clock 5000!");
    lcd.setCursor(0,2);
    lcd.print("====================");
    lcd.setCursor(5,3);
    lcd.print("Loading...");

    pinMode(D0, INPUT);

    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(100);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    unsigned long timeoutStartMillis = millis();
    while (WiFi.status() != WL_CONNECTED) {
        if (millis() - timeoutStartMillis > 10000) {
            String temp = "The connection to wifi timed out! Wifi status code: ";
            temp += WiFi.status();
            error(temp);
        }
        delay(1);
    }

    setenv("TZ", "CET-1", 1);  // The middle argument is the timezone, and it's in a very weird-ass format, but you basically need to write it like that: <RANDOM THREE LETTER CODE><A MINUS IF YOU WANT IT TO BE ON UTC+ OR NOTHING IF YOU WANT IT TO BE ON UTC-><OFFSET FROM UTC>
    tzset();
    updateTime();

    lcd.clear();
}

unsigned long beforeMillisTime = 0;
bool wasButtonPressed = false;
uint8_t currentScreen = 0;

void loop() {
    if (beforeMillisTime > millis()) {
        updateTime();
    }
    beforeMillisTime = millis();

    bool isButtonPressed = (bool) digitalRead(D0);
    if (isButtonPressed && !wasButtonPressed) {
        currentScreen++;
        if (currentScreen > 2) {
            currentScreen = 0;
        }
        lcd.clear();
    }
    wasButtonPressed = isButtonPressed;

    switch (currentScreen) {
        case 0:
            {
                time_t nowTimestamp = startTimestamp + ((millis() - startMillisTime) / 1000);
                tm* now = localtime(&nowTimestamp);
                {
                    char row[8];
                    strftime(&row[0], 9, "%H:%M:%S", now);
                    lcd.setCursor(6, 1);
                    lcd.print(row);
                }
                {
                    char row[10];
                    strftime(&row[0], 11, "%d/%m/%Y", now);
                    lcd.setCursor(5, 2);
                    lcd.print(row);
                }
                {
                    char row[20];
                    lcd.setCursor((20 - strftime(&row[0], 21, "%A, %B", now)) / 2, 3);
                    lcd.print(row);
                }
            }
            break;
        case 1:
            lcd.setCursor(0, 0);
            lcd.print("WEATHER INFO");
            break;
        case 2:
            lcd.setCursor(0, 0);
            lcd.print("CAR GAME");
    }
}
