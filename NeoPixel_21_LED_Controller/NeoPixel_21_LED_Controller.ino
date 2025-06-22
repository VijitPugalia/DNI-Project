#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <Adafruit_NeoPixel.h>
#include <Arduino_JSON.h>

// WiFi credentials
const char* ssid = "570 Lygon";
const char* password = "Pass@112";

// LED setup
#define LED_PIN 5
#define NUM_LEDS 40
Adafruit_NeoPixel strip(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

// Button pins
const int switchPin1 = 15;
const int switchPin2 = 32;
const int switchPin3 = 14;
const int switchPin4 = 20;
const int switchPin5 = 22;

// Web server and SSE
AsyncWebServer server(80);
AsyncEventSource events("/events");

// Timing for stream updates
unsigned long lastUpdate = 0;
const unsigned long updateInterval = 20;

// Convert hex to byte
uint8_t hexToByte(const String &hex) {
  return (uint8_t) strtoul(hex.c_str(), nullptr, 16);
}

// Named color to RGB
uint32_t getNamedColor(const String& name) {
  String color = name;
  color.toLowerCase();
  if (color == "red") return strip.Color(255, 0, 0);
  if (color == "green") return strip.Color(0, 255, 0);
  if (color == "blue") return strip.Color(0, 0, 255);
  if (color == "yellow") return strip.Color(255, 255, 0);
  if (color == "magenta") return strip.Color(255, 0, 255);
  if (color == "cyan") return strip.Color(0, 255, 255);
  if (color == "white") return strip.Color(255, 255, 255);
  if (color == "black" || color == "off") return strip.Color(0, 0, 0);
  if (color == "orange") return strip.Color(255, 165, 0);
  if (color == "purple") return strip.Color(128, 0, 128);
  return strip.Color(0, 0, 0);
}

// Read button states and return as JSON string
String getButtonStates() {
  JSONVar states;
  states["button1"] = digitalRead(switchPin1) == LOW ? "true" : "false";
  states["button2"] = digitalRead(switchPin2) == LOW ? "true" : "false";
  states["button3"] = digitalRead(switchPin3) == LOW ? "true" : "false";
  states["button4"] = digitalRead(switchPin4) == LOW ? "true" : "false";
  states["button5"] = digitalRead(switchPin5) == LOW ? "true" : "false";
  return JSON.stringify(states);
}

void setup() {
  Serial.begin(115200);
  strip.begin();
  strip.clear();
  strip.show();

  // Setup buttons as pullups
  pinMode(switchPin1, INPUT_PULLUP);
  pinMode(switchPin2, INPUT_PULLUP);
  pinMode(switchPin3, INPUT_PULLUP);
  pinMode(switchPin4, INPUT_PULLUP);
  pinMode(switchPin5, INPUT_PULLUP);

  // WiFi connect
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected.");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  // LED color set endpoint
  server.on("/leds", HTTP_GET, [](AsyncWebServerRequest *request) {
    for (int i = 0; i < NUM_LEDS; i++) {
      String param = "rgb" + String(i);
      if (request->hasParam(param)) {
        String value = request->getParam(param)->value();
        value.trim();
        value.toUpperCase();

        if (value.length() == 6) {
          uint8_t r = hexToByte(value.substring(0, 2));
          uint8_t g = hexToByte(value.substring(2, 4));
          uint8_t b = hexToByte(value.substring(4, 6));
          strip.setPixelColor(i, strip.Color(r, g, b));
        } else {
          strip.setPixelColor(i, getNamedColor(value));
        }
      }
    }
    strip.show();
    request->send(200, "text/plain", "LEDs updated");
  });

  // Turn off all LEDs
  server.on("/alloff", HTTP_GET, [](AsyncWebServerRequest *request) {
    strip.clear();
    strip.show();
    request->send(200, "text/plain", "All LEDs turned off");
  });

  // Serve button states as JSON
  server.on("/data", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "application/json", getButtonStates());
  });

  // Server-Sent Events
  events.onConnect([](AsyncEventSourceClient *client) {
    Serial.println("Client connected to /events");
    client->send("connected", NULL, millis(), 10000);
  });
  server.addHandler(&events);

  server.begin();
}

void loop() {
  if (millis() - lastUpdate > updateInterval) {
    String stateData = getButtonStates();
    events.send(stateData.c_str(), "button_state", millis());
    lastUpdate = millis();
  }
}
