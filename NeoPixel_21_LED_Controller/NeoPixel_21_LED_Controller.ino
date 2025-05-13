#include <Adafruit_NeoPixel.h>

#define LED_PIN     5
#define LED_COUNT   21  // Using only 20 LEDs

Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);
String input = "";

void setup() {
  Serial.begin(115200);
  strip.begin();
  strip.setBrightness(100);
  strip.show();
  Serial.println("Control 21 LEDs with:");
  Serial.println("1 red | 2 blue | 3 #00FF00 | 1 off | all off");
}

void loop() {
  if (Serial.available()) {
    input = Serial.readStringUntil('\n');
    input.trim();
    input.toLowerCase();

    if (input == "all off") {
      for (int i = 0; i < LED_COUNT; i++) {
        strip.setPixelColor(i, 0);  // Off
      }
      strip.show();
      Serial.println("All LEDs turned off.");
      return;
    }

    int spaceIndex = input.indexOf(' ');
    if (spaceIndex == -1) {
      Serial.println("Invalid input. Format: <LED#> <color>");
      return;
    }

    int ledIndex = input.substring(0, spaceIndex).toInt() - 1;  // Adjust for 1-based input
    String colorStr = input.substring(spaceIndex + 1);

    if (ledIndex < 0 || ledIndex >= LED_COUNT) {
      Serial.println("LED number must be between 1 and 20.");
      return;
    }

    if (colorStr == "off") {
      strip.setPixelColor(ledIndex, 0);
    } else if (colorStr.startsWith("#") && colorStr.length() == 7) {
      long hexColor = strtol(colorStr.substring(1).c_str(), NULL, 16);
      uint8_t r = (hexColor >> 16) & 0xFF;
      uint8_t g = (hexColor >> 8) & 0xFF;
      uint8_t b = hexColor & 0xFF;
      strip.setPixelColor(ledIndex, strip.Color(r, g, b));
    } else {
      uint32_t namedColor = getColorFromName(colorStr);
      strip.setPixelColor(ledIndex, namedColor);
    }

    strip.show();
  }
}

uint32_t getColorFromName(String name) {
  if (name == "red") return strip.Color(255, 0, 0);
  if (name == "green") return strip.Color(0, 255, 0);
  if (name == "blue") return strip.Color(0, 0, 255);
  if (name == "yellow") return strip.Color(255, 255, 0);
  if (name == "orange") return strip.Color(255, 165, 0);
  if (name == "white") return strip.Color(255, 255, 255);
  if (name == "purple") return strip.Color(128, 0, 128);
  if (name == "pink") return strip.Color(255, 105, 180);
  return 0; // Unknown color = off
}
