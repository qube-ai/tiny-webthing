#pragma once

#include <ArduinoJson.h>
#include "Thing.h"
#include <WebSocketsClient.h>

// check if logging is enabled
#ifndef TA_LOGGING
#define TA_LOG(...) (void)0
#else
#define TA_LOG(...) Serial.printf(__VA_ARGS__)
#endif

#define ARDUINOJSON_USE_LONG_LONG 1

#ifndef LARGE_JSON_DOCUMENT_SIZE
#define LARGE_JSON_DOCUMENT_SIZE 2048
#endif

#ifndef SMALL_JSON_DOCUMENT_SIZE
#define SMALL_JSON_DOCUMENT_SIZE 512
#endif

#ifndef TINY_JSON_DOCUMENT_SIZE
#define TINY_JSON_DOCUMENT_SIZE 256
#endif


class TinyAdapter
{

public:
    TinyAdapter(String _websocketUrl, int _port, String _websocketPath)
        : websocketUrl(_websocketUrl),  port(_port), websocketPath(_websocketPath) {}

    String websocketUrl;   // "eg: ws://<somehost>"
    int port; // port to connect to
    String websocketPath; // "eg: /ws"

    // Final url -> ws://<websocketUrl>:<port><websocketPath>

    ThingDevice *firstDevice = nullptr;
    ThingDevice *lastDevice = nullptr;
    WebSocketsClient webSocket;

    WebSocketsClient getWebSocketClient()
    {
        return webSocket;
    }

    /*
     * Send a message to the server
     * @param {String} message
     */
    void sendMessage(String &msg)
    {
        webSocket.sendTXT(msg.c_str(), msg.length() + 1);
    }

    /*
     * Handles the message received from the server.
     * @param {String} payload
     */
    void messageHandler(String payload)
    {
        StaticJsonDocument<TINY_JSON_DOCUMENT_SIZE> doc;
        DeserializationError error = deserializeJson(doc, payload);
        if (error)
        {
            TA_LOG("[TA:messageHandler] deserializeJson() failed: %s\n", error.c_str());
            String msg = "{\"messageType\":\"error\", \"errorMessage\":\"deserializeJson() failed \"}";
            sendMessage(msg);
        }

        JsonObject root = doc.as<JsonObject>();

        if (root["messageType"] == "getProperty")
        {
            TA_LOG("[TA:messageHandler] Received a 'getProperty' message\n");
            String thingId = root["thingId"];
            getProperties(thingId);
        }

        else if (root["messageType"] == "setProperty")
        {
            TA_LOG("[TA:messageHandler] Received a 'setProperty' message\n");
            String thingId = root["thingId"];
            String propertyId = root["data"]["propertyId"];
            String data = root["data"];
            setProperty(thingId, propertyId, data);
        }

        else if (root["messageType"] == "getAllThings")
        {
            TA_LOG("[TA:messageHandler] Received a 'getAllThings' message\n");
            getThingDescription();
        }

        else
        {
            TA_LOG("[TA:messageHandler] Unknown message type received. Payload: %s\n", payload.c_str());
        }
    }

    /*
     * Callback for websocket events.
     */
    void webSocketEvent(WStype_t type, uint8_t *payload, size_t length)
    {
        switch (type)
        {
        case WStype_DISCONNECTED:
            TA_LOG("[TA:webSocketEvent] Disconnect!\n");
            break;

        case WStype_CONNECTED:
            TA_LOG("[TA:webSocketEvent] Connected to tunnel server!\n");
            webSocket.sendTXT("{\"messageType\":\"StartWs\"}");
            break;

        case WStype_TEXT:
        {
            TA_LOG("[TA:payloadHandler] New message received!\n");
            char msgch[length];
            for (unsigned int i = 0; i < length; i++)
            {
                msgch[i] = ((char)payload[i]);
            }
            msgch[length] = '\0';
            String msg = msgch;
            messageHandler(msg);
            break;
        }

        case WStype_ERROR:
            TA_LOG("[TA:webSocketEvent] Error!\n");
            break;

        case WStype_FRAGMENT_TEXT_START:
            TA_LOG("[TA:webSocketEvent] Fragment Text Start!\n");
            break;

        case WStype_FRAGMENT_BIN_START:
            TA_LOG("[TA:webSocketEvent] Fragment Bin Start!\n");
            break;

        case WStype_FRAGMENT:
            TA_LOG("[TA:webSocketEvent] Fragment!\n");
            break;

        case WStype_FRAGMENT_FIN:
            TA_LOG("[TA:webSocketEvent] Fragment Fin!\n");
            break;

        case WStype_PING:
            TA_LOG("[TA:webSocketEvent] Ping!\n");
            break;

        case WStype_PONG:
            TA_LOG("[TA:webSocketEvent] Pong!\n");
            break;
        }
    }

    /*
     * Find a thing by its id
     * @param {String} thingId
     * @return {TinyThing} thing
     */
    ThingDevice *findDeviceById(String id)
    {
        ThingDevice *device = this->firstDevice;
        while (device != nullptr)
        {
            if (device->id == id)
            {
                return device;
            }
            device = device->next;
        }
        return nullptr;
    }

    /*
     * Find a property by its id
     * @param {String} propertyId
     * @return {TinyProperty} property
     */
    ThingProperty *findPropertyById(ThingDevice *device, String id)
    {
        ThingProperty *property = device->firstProperty;
        while (property != nullptr)
        {
            if (property->id == id)
            {
                return property;
            }
            property = (ThingProperty *)property->next;
        }
        return nullptr;
    }

    /*
     * Setup a unsecure websocket connection.
     * @param String websocketUrl @param int websocketPort, @param String websocketPath
     */
    void begin()
    {

        // server address, port and URL
        webSocket.begin(websocketUrl, port, websocketPath);
        // event handler
        webSocket.onEvent(std::bind(
            &TinyAdapter::webSocketEvent, this, std::placeholders::_1,
            std::placeholders::_2, std::placeholders::_3));
    }

    /*
     * Setup a secure websocket connection.
     * @param String websocketUrl @param int websocketPort, @param String websocketPath
     */
    void beginSSL()
    {

        // server address, port and URL
        webSocket.beginSSL(websocketUrl.c_str(), port, websocketPath.c_str());
        // event handler
        webSocket.onEvent(std::bind(
            &TinyAdapter::webSocketEvent, this, std::placeholders::_1,
            std::placeholders::_2, std::placeholders::_3));
    }

    /*
     * Updates websocket connection and send changed properties to the server.
     * Note: this method should be called in the loop.
     */
    void update()
    {

        webSocket.loop();
        ThingDevice *device = this->firstDevice;
        while (device != nullptr)
        {
            sendChangedProperties(device);
            device = device->next;
        }
    }

    /*
     * Add a thing to the adapter.
     * @param TinyThing* thing
     */
    void addDevice(ThingDevice *device)
    {
        if (this->lastDevice == nullptr)
        {
            this->firstDevice = device;
            this->lastDevice = device;
        }
        else
        {
            this->lastDevice->next = device;
            this->lastDevice = device;
        }
    }

    /*
     * Send all properties value to the server that have been changes.
     * @param ThingDevice *device
     */
    void sendChangedProperties(ThingDevice *device)
    {
        StaticJsonDocument<TINY_JSON_DOCUMENT_SIZE> message;
        message["messageType"] = "propertyStatus";
        JsonObject prop = message.createNestedObject("data");
        bool dataToSend = false;
        ThingItem *item = device->firstProperty;
        while (item != nullptr)
        {
            ThingDataValue *value = item->changedValueOrNull();
            if (value)
            {
                dataToSend = true;
                item->serializeValue(prop);
            }
            item = item->next;
        }
        if (dataToSend)
        {
            String jsonStr;
            message["thingId"] = device->id;
            serializeJson(message, jsonStr);
            sendMessage(jsonStr);
        }
    }

    /*
     * When server asks for thing description this method is called.
     * Generates a thing description and sends it to the server.
     */
    void getThingDescription()
    {
        // StaticJsonDocument<LARGE_JSON_DOCUMENT_SIZE> buf;
        DynamicJsonDocument buf(LARGE_JSON_DOCUMENT_SIZE);
        JsonArray things = buf.to<JsonArray>();
        ThingDevice *device = this->firstDevice;
        while (device != nullptr)
        {
            JsonObject descr = things.createNestedObject();
            device->serialize(descr);
            descr["href"] = "/things/" + device->id;
            device = device->next;
        }
        // StaticJsonDocument<LARGE_JSON_DOCUMENT_SIZE> doc;
        DynamicJsonDocument doc(LARGE_JSON_DOCUMENT_SIZE);
        JsonObject doc2 = doc.to<JsonObject>();
        doc2["messageType"] = "descriptionOfThings";
        doc2["things"] = things;
        String jsonStr;
        serializeJson(doc2, jsonStr);
        sendMessage(jsonStr);
    }

    /*
     * When server asks for property value this method is called.
     * Serializes all properties and send it to the server.
     * @example { "temperature": 23, "humidity": 45 }
     */
    void getProperties(String thingId)
    {
        ThingItem *rootItem = findDeviceById(thingId)->firstProperty;
        if (rootItem == nullptr)
        {
            String msg = "{\"messageType\":\"error\",\"errorCode\":\"404\",\"errorMessage\":\"Thing not found\", \"thingId\": \"" + thingId + "\"}";
            sendMessage(msg);
        }

        StaticJsonDocument<SMALL_JSON_DOCUMENT_SIZE> doc;
        JsonObject prop = doc.to<JsonObject>();
        ThingItem *item = rootItem;
        while (item != nullptr)
        {
            item->serializeValue(prop);
            item = item->next;
        }
        StaticJsonDocument<SMALL_JSON_DOCUMENT_SIZE> finalDoc;
        JsonObject finalProp = finalDoc.to<JsonObject>();
        finalDoc["messageType"] = "getProperty";
        finalDoc["thingId"] = thingId;
        finalDoc["properties"] = prop;
        String jsonStr;
        serializeJson(finalProp, jsonStr);
        sendMessage(jsonStr);
        TA_LOG("[TA:getProperties] Property data was sent back.\n");
    }

    /*
     * Change a property value.
     * @param {String} thingId
     * @param {String} propertyId
     * @param {String} newPropertyData - {"propertyId": "value"}
     * 
     */
    void setProperty(String thingId, String propertyId, String newPropertyData)
    {

        ThingDevice *device = findDeviceById(thingId);
        if (device == nullptr)
        {
            TA_LOG("[TA:setProperty] Thing not found. %s \n", thingId.c_str());
            return;
        }
        ThingProperty *property = findPropertyById(device, propertyId);
        if (property == nullptr)
        {   
            TA_LOG("[TA:setProperty] Property not found. %s \n", propertyId.c_str());
            return;
        }

        StaticJsonDocument<SMALL_JSON_DOCUMENT_SIZE> newBuffer;
        auto error = deserializeJson(newBuffer, newPropertyData);

        if (error)
        {
            TA_LOG("[TA:setProperty] Parsing json error. %s \n", error.c_str());
            return;
        }
        
        TA_LOG("[TA:setProperty] Property data was received. %s \n", newPropertyData.c_str());
        JsonObject newProp = newBuffer.as<JsonObject>();
        device->setProperty(property->id.c_str(), newProp["value"]);
        // Don't send the value back to the server
        // The update method will send the changed properties
        TA_LOG("[TA:setProperty] Property value has been set! \n");
    }
};
