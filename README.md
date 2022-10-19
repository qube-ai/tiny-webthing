# tiny-webthing

 A tiny Web Thing implementation for Arduino. This is library does not support events, actions, or multiple things. The library uses websocket for communication. So you have to create a websocket server to be able to communicate.


## TODO

- [ ] Remove dependency on ArduinoJson
- [ ] Reduce size of TD 
- [X] Use StaticJsonDocument instead of DynamicJsonDocument
- [ ] Fixed size of messages received from server
- [ ] Add support for multiple things
- [ ] Change message schema in tunnel server and client
 

## Example

```c++
#include <Arduino.h>
#include <Thing.h>
#include <QubeAdapter.h>

const char *ssid = "WillowCove";
const char *password = "Deepwaves007";

const int ledPin = LED_BUILTIN;

QubeAdapter *adapter;

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
  adapter = new QubeAdapter();
  led.addProperty(&ledOn);
  led.addProperty(&ledBrightness);
  adapter->addDevice(&led);
  // Address of your web  socket server goes here
  // For non SSL use adapter->begin()
  adapter->beginSSL("tunnel.qube.eco", 80, "/ws");
  Serial.println("HTTP server started");
}

void loop()
{
  adapter->update();
}


```




