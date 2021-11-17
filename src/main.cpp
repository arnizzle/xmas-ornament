/**
 * ----------------------------------------------------------------------------
 * ESP32 Remote Control with WebSocket
 * ----------------------------------------------------------------------------
 * © 2020 Stéphane Calderoni
 * ----------------------------------------------------------------------------
 */

#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <ESPAsyncTCP.h>
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <Syslog.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include "Ornament.h"

// ----------------------------------------------------------------------------
// Definition of Time delays 
// ----------------------------------------------------------------------------

long alternateDelay = 5000;
unsigned long alternateTimeNow = 0;

bool debugMode = false;
bool alternateMode = false;
bool randomMode = false;

// ----------------------------------------------------------------------------
// Definition of Ornaments
// ----------------------------------------------------------------------------

ornament Train;             // button 1
ornament Snowman;           // button 2

// ----------------------------------------------------------------------------
// Definition of Sliders
// ----------------------------------------------------------------------------

int slider1 = 0;
int slider2 = 0;

// ----------------------------------------------------------------------------
// Definition of macros
// ----------------------------------------------------------------------------

#define LED_PIN   26
#define BTN_PIN   22
#define HTTP_PORT 80

// ----------------------------------------------------------------------------
// Definition of global constants
// ----------------------------------------------------------------------------

// Button debouncing
const uint8_t DEBOUNCE_DELAY = 10; // in milliseconds

// WiFi credentials
const char *WIFI_SSID = "127.0.0.1";
const char *WIFI_PASS = "4Xoozzop";


// ----------------------------------------------------------------------------
// Syslog init
// ----------------------------------------------------------------------------

// Syslog server connection info
#define SYSLOG_SERVER "172.16.10.100"
#define SYSLOG_PORT 514

// This device info
#define DEVICE_HOSTNAME "testbox"
#define APP_NAME "xmas-app"


WiFiUDP udpClient;
Syslog syslog(udpClient, SYSLOG_SERVER, SYSLOG_PORT, DEVICE_HOSTNAME, APP_NAME, LOG_KERN);


// ----------------------------------------------------------------------------
// Definition of the LED component
// ----------------------------------------------------------------------------

struct Led {
    // state variables
    uint8_t pin;
    bool    on;

    // methods
    void update() {
        digitalWrite(pin, on ? HIGH : LOW);
    }
};

// ----------------------------------------------------------------------------
// Logging component / Syslog
// ----------------------------------------------------------------------------

void doLog(String message) {
    // String fullMessage = "[Today] [XMAS-Server TEST" + message;

    // int iteration = 10;
    // syslog.logf(LOG_ERR,  "This is error message no. %d", iteration);
    char Buf[100];
    message.toCharArray(Buf, 100);
    syslog.logf(LOG_ERR, Buf);


    // if ( fullMessage.length() < 80 ) {
    //     char Buf[100];
    //     fullMessage.toCharArray(Buf, 100);

    //     udpClient.beginPacket(SYSLOG_SERVER,SYSLOG_PORT);
    //     udpClient.printf(Buf);
    //     udpClient.endPacket();  
    // }
    Serial.println (message);
}

// ----------------------------------------------------------------------------
// Definition of global variables
// ----------------------------------------------------------------------------

Led    onboard_led = { LED_BUILTIN, false };
Led    led         = { LED_PIN, false };

AsyncWebServer server(HTTP_PORT);
AsyncWebSocket ws("/ws");

// ----------------------------------------------------------------------------
// SPIFFS initialization
// ----------------------------------------------------------------------------

void initSPIFFS() {
  if (!SPIFFS.begin()) {
    Serial.println("Cannot mount SPIFFS volume...");
    while (1) {
        onboard_led.on = millis() % 200 < 50;
        onboard_led.update();
    }
  }
}

// ----------------------------------------------------------------------------
// Connecting to the WiFi network
// ----------------------------------------------------------------------------

void initWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.printf("Trying to connect [%s] ", WiFi.macAddress().c_str());
  while (WiFi.status() != WL_CONNECTED) {
      Serial.print(".");
      delay(500);
  }
  Serial.printf(" %s\n", WiFi.localIP().toString().c_str());
}

// ----------------------------------------------------------------------------
// Web server initialization
// ----------------------------------------------------------------------------

// String processor(const String &var) {
//     return String(var == "STATE" && led.on ? "on" : "off");
// }

// Replaces placeholder with stored values


String processor(const String& var){
    if (var == "button1") {
        return (Snowman.ledStatus == true) ? "on" : "off";
    }
    if (var == "button2") {
        return (Train.ledStatus == true) ? "on" : "off";
    }
    if (var == "button3") {
        return (randomMode == true) ? "on" : "off";
    }
    if (var == "button4") {
        return (alternateMode == true) ? "on" : "off";
    }
    if (var == "button5") {
        return (debugMode == true) ? "on" : "off";
    }

}

void onRootRequest(AsyncWebServerRequest *request) {
  request->send(SPIFFS, "/index.html", "text/html", false, processor);
}

void initWebServer() {
    server.on("/", onRootRequest);
    server.serveStatic("/", SPIFFS, "/");
    server.begin();
}

// ----------------------------------------------------------------------------
// WebSocket initialization
// ----------------------------------------------------------------------------

void notifyClients() {
    doLog("Notifying CLients");

    const uint16_t size = JSON_OBJECT_SIZE(6);
    StaticJsonDocument<size> json;
    
    ( Snowman.ledStatus ) ? json["button1_status"] = "on" : json["button1_status"] = "off";
    ( Train.ledStatus  ) ? json["button2_status"] = "on" : json["button2_status"] = "off";
    ( randomMode ) ? json["button3_status"] = "on" : json["button3_status"] = "off";
    ( alternateMode ) ? json["button4_status"] = "on" : json["button4_status"] = "off";
    ( debugMode ) ? json["button5_status"] = "on" : json["button5_status"] = "off";
    
     String jsonBody;
    doLog(jsonBody);

    serializeJson(json, jsonBody);

    char buffer[150];

    doLog("Notifying Clients....");

    size_t len = serializeJson(json, buffer);
    ws.textAll(buffer, len);
}

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
    doLog("Handle Websocket msg");
    Serial.println("Handle websockets");

    AwsFrameInfo *info = (AwsFrameInfo*)arg;
    if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {

        // const uint8_t size = JSON_OBJECT_SIZE(2);
        const uint16_t size = JSON_OBJECT_SIZE(10);
        
        StaticJsonDocument<size> json;
        
        DeserializationError err = deserializeJson(json, data);
        
        if (err) {
            Serial.print(F("deserializeJson() failed with code "));
            Serial.println(err.c_str());
            return;
        }

        const char *action = json["action"];
        const char *value = json["value"];

        if (strcmp(action, "toggle1") == 0) {
            Snowman.toggle();
            doLog("Snowman Toggle");
            alternateMode = false;
            randomMode = false;
            notifyClients();
        }

        if (strcmp(action, "toggle2") == 0) {
            Train.toggle();
            doLog("Train Toggle");
            notifyClients();
            alternateMode = false;
            randomMode = false;
        }

        if (strcmp(action, "toggle3") == 0) {
            if (randomMode) {
                randomMode = false;
            } else {
                randomMode = true;
                alternateMode = false;
            }
            notifyClients();
        }

        if (strcmp(action, "toggle4") == 0) {
            if (alternateMode) {
                alternateMode = false;
            } else {
                alternateMode = true;
                Snowman.turnOn();
                Train.turnOff();
                randomMode = false;
            }
            notifyClients();
        }
        if (strcmp(action, "toggle5") == 0) {
            debugMode =! debugMode;
            if (debugMode) {            
                alternateDelay = 5000;
            } else {
                alternateDelay = 500000;
            }
            Train.setInterval(debugMode);
            Snowman.setInterval(debugMode);
            notifyClients();
        }
    }
}

void onEvent(AsyncWebSocket       *server,
             AsyncWebSocketClient *client,
             AwsEventType          type,
             void                 *arg,
             uint8_t              *data,
             size_t                len) {

    switch (type) {
        case WS_EVT_CONNECT:
            Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
            break;
        case WS_EVT_DISCONNECT:
            Serial.printf("WebSocket client #%u disconnected\n", client->id());
            break;
        case WS_EVT_DATA:
            handleWebSocketMessage(arg, data, len);
            break;
        case WS_EVT_PONG:
        case WS_EVT_ERROR:
            break;
    }
}

void initWebSocket() {
    ws.onEvent(onEvent);
    server.addHandler(&ws);
}

// ----------------------------------------------------------------------------
// Initialization
// ----------------------------------------------------------------------------

void setup() {
    // OTA?
    ArduinoOTA.begin();

    Train.LED = 14;
    Train.setInterval(debugMode);
    Train.Setup();
    
    Snowman.LED = 4;
    Snowman.setInterval(debugMode);
    Snowman.Setup();
    
    Serial.begin(115200); delay(500);

    initSPIFFS();
    initWiFi();
    initWebSocket();
    initWebServer();


}

// ----------------------------------------------------------------------------
// Main control loop
// ----------------------------------------------------------------------------

void loop() {
    ws.cleanupClients();
    ArduinoOTA.handle();
    
    if (randomMode) {
        if (Snowman.checkStatus(debugMode) || Train.checkStatus(debugMode) ) {
            notifyClients();
            Serial.println('NOTIFYING CLIENTS FROM RANDOMMODE');
        }
    }

    if (alternateMode) {
        if(millis() >= alternateTimeNow + alternateDelay) {
            alternateTimeNow = millis();
            Snowman.toggle();
            Train.toggle();
            notifyClients();
        }
    }

    delay(500);
}
