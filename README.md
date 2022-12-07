# tiny-webthing

A simple library for the ESP8266 that implements Mozilla's proposed Web of Things API. This library is a port of [webthing-arduino](https://github.com/WebThingsIO/webthing-arduino) but this library is much smaller and simpler. Instead of creating a server on the ESP8266 and communicate locally, this library uses websocket to tunnel the ESP8266/thing and communicate with it over the internet. So to use this library you need to write a tunnel server that will communicate with the ESP8266/thing over websocket with following schema as mentioned below.


## Installation (Platformio)
  - From local lib
    ```
    lib_deps = file:///path/to/lib/tiny-webthing
    ```
  - From Git
    ```
    lib_deps = https://github.com/qube-ai/tiny-webthing.git
    ```

## SSL
- For ssl use `beginSSL()` method of TinyAdapter.


## Message Schema
![Schema](https://img.shields.io/badge/Schema-Tiny%20Things-blue.svg)

This is the schema used by the library to communicate with the server. So when you write a tunnel server, you have to follow this schema.

### Message sent by the Tunnel -> Thing


> These are the only messageType(s) allowed by the library, and any other types will not pe processed.

- **Set Property**: This message is used to set any property of the thing.
eg:
```json
{
  "messageType": "setProperty",
  "thingId": "thingId",
  "data": {
    "propertyId": "propertyId",
    "newValue": "newValue",
  }
}
```

- **Get thing description**: This message is used to get the thing description.
```json
{
  "messageType": "getAllThings",
}
```

- **Get Property**: This will send the current value of all the properties.
```js
{
  "messageType": "getProperty",
  "thingId": "thingId",
}

response :
{
  "messageType": "getProperty",
  "thingId": <thingId>,
  "properties": {
    <propertyId>: <value>,
    .
    .
    .
    <propertyId>: <value>,
  }
}
```

### Message sent by the Thing -> Tunnel Server

> These are the only messageType(s) that the library will/can send to tunnel server.

- **StartWs**: This message is sent by the library when the websocket connection is established.
```json
{
  "messageType": "startWs",
}
``` 

- **Property Updates**: This message is sent by the library whenever any property changed. This is async, i.e. if and when any property changed this message will be sent by the library as update.
```json
{
  "messageType": "propertyStatus",
  "thingId": "thingId",
  "data": {
    "propertyId": "propertyId",
    "value": "value",
  }
}
```

- **Description Of Things**: This message is sent by the library whenever the server asks for `getAllThings`.
```js
{
  "messageType": "descriptionOfThings",
  "things": [<TD>]
}
```

- **Get Property**: This message is sent by the library whenever the server asks for `getProperty`.
```js
{
  "messageType": "getProperty",
  "thingId": <thingId>,
  "properties": {
    <propertyId>: <value>,
    .
    .
    .
    <propertyId>: <value>,
  }
}
```

## Example

```C++
#include <Arduino.h>
#include <Thing.h>
#include <TinyAdapter.h>

const char *ssid = "WillowCove";
const char *password = "Deepwaves007";

const int ledPin = LED_BUILTIN;

TinyAdapter *adapter;

void onOffChanged(ThingPropertyValue newValue);

// Define the types of the thing. This will be `@type` in the thing description.
const char *ledTypes[] = {"OnOffSwitch", "LightBrightnesControl", nullptr};
ThingDevice led("tiny-thing-02", ledTypes);
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


```


## Configuration
If you have a complex device with large thing descriptions, you may need to increase the size of the JSON buffers. The buffer sizes are configurable as such:

```cpp
// By default, buffers are 256 for tiny, 512 bytes for small documents, 2048 for larger ones
// You can change these by defining them before including the library
#define LARGE_JSON_DOCUMENT_SIZE 2048
#define SMALL_JSON_DOCUMENT_SIZE 512
#define TINY_JSON_DOCUMENT_SIZE 256

#include <Thing.h>
#include <TinyAdapter.h>

```

- Enable debug mode by defining `TA_LOGGING` before including the library.

```cpp
#define TA_LOGGING 1

#include <Thing.h>
#include <TinyAdapter.h>

```


## Architecture

![Architecture](https://img.shields.io/badge/Architecture-Tiny%20Things-blue.svg)

![Architecture](/docs/tiny-webthing-arch.png)

## TODO

- [x] Use StaticJsonDocument instead of DynamicJsonDocument
- [ ] Remove dependency on ArduinoJson
- [ ] Reduce size of TD
- [ ] Fixed size of messages received from server
- [ ] Change message schema in tunnel server and client
- [ ] Add support for actions, events


