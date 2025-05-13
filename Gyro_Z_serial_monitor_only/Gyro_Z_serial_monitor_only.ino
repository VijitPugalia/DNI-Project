#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Arduino_JSON.h>
#include "LittleFS.h"

// WiFi credentials
const char* ssid = "570 Lygon";
const char* password = "Pass@112";

// Server and sensor
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
  Serial.print("Connected! IP address: ");
  Serial.println(WiFi.localIP());
}

void initMPU() {
  if (!mpu.begin()) {
    Serial.println("Failed to find MPU6050 chip");
    while (1) delay(10);
  }
  Serial.println("MPU6050 Found!");
}

void initLittleFS() {
  if (!LittleFS.begin()) {
    Serial.println("LittleFS mount failed");
  } else {
    Serial.println("LittleFS mounted successfully");
  }
}

String getGyroZReading() {
  mpu.getEvent(&a, &g, &temp);
  float zRate = g.gyro.z;
  if (abs(zRate) > gyroZerror) {
    gyroZ += zRate / 50.0;
  }

  Serial.print("Gyro Z: ");
  Serial.println(gyroZ);

  readings["gyroZ"] = String(gyroZ);
  return JSON.stringify(readings);
}

void setup() {
  Serial.begin(115200);
  initWiFi();
  initLittleFS();
  initMPU();

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(LittleFS, "/index.html", "text/html");
  });

  server.serveStatic("/", LittleFS, "/");

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
