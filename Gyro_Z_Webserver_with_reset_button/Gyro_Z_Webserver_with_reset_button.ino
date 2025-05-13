#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Arduino_JSON.h>
#include "LittleFS.h"

const char* ssid = "570 Lygon";
const char* password = "Pass@112";

AsyncWebServer server(80);
AsyncEventSource events("/events");

Adafruit_MPU6050 mpu;
sensors_event_t a, g, temp;

float gyroZ = 0;
float gyroZerror = 0.01;
unsigned long lastTime = 0;
unsigned long gyroDelay = 50;
JSONVar readings;

void initWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(1000);
  }
  Serial.println();
  Serial.print("ESP32 Web Server IP: ");
  Serial.println(WiFi.localIP());
}

void initMPU() {
  if (!mpu.begin()) {
    Serial.println("Failed to find MPU6050 chip");
    while (1) delay(10);
  }
}

void initLittleFS() {
  if (!LittleFS.begin()) {
    Serial.println("LittleFS mount failed");
  }
}

const int switchPin1 = 27;
const int switchPin2 = 33;
const int switchPin3 = 32;

String getGyroZReading() {
  mpu.getEvent(&a, &g, &temp);
  float zRate = g.gyro.z;
  if (abs(zRate) > gyroZerror) {
    gyroZ += zRate / 50.0;
  }
  readings["gyroZ"] = String(gyroZ);
  readings["button1"] = digitalRead(switchPin1) == LOW ? "true" : "false";
  readings["button2"] = digitalRead(switchPin2) == LOW ? "true" : "false";
  readings["button3"] = digitalRead(switchPin3) == LOW ? "true" : "false";
  return JSON.stringify(readings);
}

void setup() {
  pinMode(switchPin1, INPUT_PULLUP);
  pinMode(switchPin2, INPUT_PULLUP);
  pinMode(switchPin3, INPUT_PULLUP);

  Serial.begin(115200);
  initWiFi();
  initLittleFS();
  initMPU();

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(LittleFS, "/index.html", "text/html");
  });

  server.serveStatic("/", LittleFS, "/");

  server.on("/resetZ", HTTP_GET, [](AsyncWebServerRequest *request) {
    gyroZ = 0;
    request->send(200, "text/plain", "Gyro Z Reset");
  });

  // ✅ Added this line to serve gyroZ data as JSON
  server.on("/data", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "application/json", getGyroZReading());
  });

  events.onConnect([](AsyncEventSourceClient *client) {
    client->send("hello!", NULL, millis(), 10000);
  });

  server.addHandler(&events);
  server.begin();
}

void loop() {
  if ((millis() - lastTime) > gyroDelay) {
    events.send(getGyroZReading().c_str(), "gyro_reading", millis());
    lastTime = millis();
  }
}
