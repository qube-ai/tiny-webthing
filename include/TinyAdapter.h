#pragma once

#ifndef TinyAdapter_h
#define TinyAdapter_h

#include <Arduino.h>
#include <ArduinoJson.h>
#include "TinyThing.h"
#include <WebSocketsClient.h>

#define LARGE_JSON_OBJECT 1024
#define SMALL_JSON_OBJECT 1024

#define TA_LOG(...) Serial.printf(__VA_ARGS__)

class TinyAdapter
{
public:
    const char *tunnelUrl;
    TinyAdapter(const char *_tunnelUrl);
    TinyThing *firstThing = nullptr;
    TinyThing *lastThing = nullptr;
    WebSocketsClient webSocket;

    /*
     * Send a message to the server
     * @param {String} message
     */
    void sendMessage(String &msg);

    /*
     * Handles the message received from the server.
     * @param {String} payload
     */
    void messageHandler(String payload);

    /*
     * Callback for websocket events.
     */
    void onWebSocketEvent(WStype_t type, uint8_t *payload, size_t length);

    /*
     * Find a thing by its id
     * @param {String} thingId
     * @return {TinyThing} thing
     */
    TinyThing *findThingById(String id);

    /*
     * Find a property by its id
     * @param {String} propertyId
     * @return {TinyProperty} property
     */
    TinyProperty *findPropertyById(TinyThing *thing, String id);

    /*
     * Setup a unsecure websocket connection.
     * @param String websocketUrl @param int websocketPort, @param String websocketPath
     */
    void begin(String websocketUrl, int websocketPort, String websocketPath);

    /*
     * Setup a secure websocket connection.
     * @param String websocketUrl @param int websocketPort, @param String websocketPath
     */
    void beginSSL(String websocketUrl, int websocketPort, String websocketPath);

    /*
     * Updates websocket connection and send changed properties to the server.
     * Note: this method should be called in the loop.
     */
    void update();

    /*
     * Add a thing to the adapter.
     * @param TinyThing* thing
     */
    void addThing(TinyThing *thing);

    /*
     * Send a property value to the server.
     * @param TinyProperty property
     */
    void sendChangedProperties(TinyThing *thing);

    /*
     * When server asks for thing description this method is called.
     * Generates a thing description and sends it to the server.
     */
    void getThingDescription();

    /*
     * When server asks for property value this method is called.
     * Serializes all properties and send it to the server.
     * @example { "temperature": 23, "humidity": 45 }
     */
    void getProperties(String thingId);

    /*
     * Change a property value.
     * @param {String} thingId
     * @param {String} propertyId
     * @param {String} value
     */
    void setProperty(String thingId, String propertyId, String value);
};

#endif