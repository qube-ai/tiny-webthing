#include <Arduino.h>
#include <Thing.h>
#include <TinyAdapter.h>

const char *ssid = "WillowCove";
const char *password = "Deepwaves007";

const int ledPin = LED_BUILTIN;

TinyAdapter *adapter;

void onOffChanged(ThingPropertyValue newValue);
ThingDevice led("tiny-thing-02");
ThingProperty ledOn("on", BOOLEAN, "OnOffProperty", onOffChanged);
ThingProperty ledBrightness("brightness", NUMBER, "BrightnessProperty", nullptr);

bool lastOn = false;

void onOffChanged(ThingPropertyValue newValue)
{
  Serial.print("On/Off changed to : ");
  Serial.println(newValue.boolean);
  digitalWrite(ledPin, newValue.boolean ? LOW : HIGH);
}

void setup(void)
{
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, HIGH);
  Serial.begin(115200);
  Serial.println("");
  Serial.print("Connecting to \"");
  Serial.print(ssid);
  Serial.println("\"");
#if defined(ESP8266) || defined(ESP32)
  WiFi.mode(WIFI_STA);
#endif
  WiFi.begin(ssid, password);
  Serial.println("");

  // Wait for connection
  bool blink = true;
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
    digitalWrite(ledPin, blink ? LOW : HIGH); // active low led
    blink = !blink;
  }
  digitalWrite(ledPin, HIGH); // active low led
  adapter = new TinyAdapter("localhost", 5555, "/ws"); // Break your tunnel url this way -> ws://localhost:5555/ws
  led.addProperty(&ledOn);
  led.addProperty(&ledBrightness);
  adapter->addDevice(&led);
  // Address of your web  socket server goes here
  // For non SSL use adapter->begin()
  adapter->beginSSL();
  Serial.println("HTTP server started");
}

void loop()
{
  adapter->update();
}