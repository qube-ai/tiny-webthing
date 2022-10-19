#pragma once

#include <ArduinoJson.h>
#include "Thing.h"
#include <WebSocketsClient.h>

#define QA_LOG(...) Serial.printf(__VA_ARGS__)

#define LARGE_JSON_DOCUMENT_SIZE 4096
#define SMALL_JSON_DOCUMENT_SIZE 1024

class QubeAdapter
{

public:
    QubeAdapter(uint16_t _port = 80,
                bool _disableHostValidation = false)
        : port(_port),
          disableHostValidation(_disableHostValidation) {}

    String name;
    String ip;
    uint16_t port;
    bool disableHostValidation;
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

        DynamicJsonDocument doc(LARGE_JSON_DOCUMENT_SIZE);
        DeserializationError error = deserializeJson(doc, payload);
        if (error)
        {
            QA_LOG("[QA:messageHandler] deserializeJson() failed: %s\n", error.c_str());
            String msg = "{\"messageType\":\"error\", \"errorMessage\":\"deserializeJson() failed \"}";
            sendMessage(msg);
        }

        JsonObject root = doc.as<JsonObject>();

        if (root["messageType"] == "getProperty")
        {
            QA_LOG("[QA:messageHandler] Received a 'getProperty' message\n");
            String thingId = root["thingId"];
            handleThingPropertiesGet(thingId);
        }

        else if (root["messageType"] == "setProperty")
        {
            String thingId = root["thingId"];
            String propertyId = root["data"]["propertyId"];
            QA_LOG("[QA:messageHandler] Received a 'setProperty' message\n");
            String data = root["data"];
            handleThingPropertyPut(thingId, propertyId, data);
        }

        else if (root["messageType"] == "getThingDescription")
        {
            QA_LOG("[QA:messageHandler] Received a 'getThingDescription' message\n");
            String thingId = root["thingId"];
            handleThing(thingId);
        }

        else if (root["messageType"] == "getAllThings")
        {
            QA_LOG("[QA:messageHandler] Received a 'getAllThings' message\n");
            handleThings();
        }

        else
        {
            QA_LOG("[QA:messageHandler] Unknown message type received. Payload: %s\n", payload.c_str());
        }
    }

    void payloadHandler(uint8_t *payload, size_t length)
    {
        QA_LOG("[QA:payloadHandler] New message received!\n");
        char msgch[length];
        for (unsigned int i = 0; i < length; i++)
        {
            msgch[i] = ((char)payload[i]);
        }
        msgch[length] = '\0';
        String msg = msgch;
        messageHandler(msg);
    }

    /*
     * Callback for websocket events.
     */
    void webSocketEvent(WStype_t type, uint8_t *payload, size_t length)
    {
        switch (type)
        {
        case WStype_DISCONNECTED:
            QA_LOG("[QA:webSocketEvent] Disconnect!\n");
            break;

        case WStype_CONNECTED:
            QA_LOG("[QA:webSocketEvent] Connected to tunnel server!\n");
            webSocket.sendTXT("{\"messageType\":\"StartWs\"}");
            break;

        case WStype_TEXT:
            payloadHandler(payload, length);
            break;

        case WStype_ERROR:
            QA_LOG("[QA:webSocketEvent] Error!\n");
            break;

        case WStype_FRAGMENT_TEXT_START:
            QA_LOG("[QA:webSocketEvent] Fragment Text Start!\n");
            break;

        case WStype_FRAGMENT_BIN_START:
            QA_LOG("[QA:webSocketEvent] Fragment Bin Start!\n");
            break;

        case WStype_FRAGMENT:
            QA_LOG("[QA:webSocketEvent] Fragment!\n");
            break;

        case WStype_FRAGMENT_FIN:
            QA_LOG("[QA:webSocketEvent] Fragment Fin!\n");
            break;

        case WStype_PING:
            QA_LOG("[QA:webSocketEvent] Ping!\n");
            break;

        case WStype_PONG:
            QA_LOG("[QA:webSocketEvent] Pong!\n");
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
    void begin(String websocketUrl, int websocketPort, String websocketPath)
    {

        // server address, port and URL
        webSocket.begin(websocketUrl, websocketPort, websocketPath);
        // event handler
        webSocket.onEvent(std::bind(
            &QubeAdapter::webSocketEvent, this, std::placeholders::_1,
            std::placeholders::_2, std::placeholders::_3));
    }

    /*
     * Setup a secure websocket connection.
     * @param String websocketUrl @param int websocketPort, @param String websocketPath
     */
    void beginSSL(String websocketUrl, int websocketPort, String websocketPath)
    {

        // server address, port and URL
        webSocket.beginSSL(websocketUrl.c_str(), websocketPort, websocketPath.c_str());
        // event handler
        webSocket.onEvent(std::bind(
            &QubeAdapter::webSocketEvent, this, std::placeholders::_1,
            std::placeholders::_2, std::placeholders::_3));
    }

    /*
     * Updates websocket connection and send changed properties to the server.
     * Note: this method should be called in the loop.
     */
    void update()
    {

        webSocket.loop();
#ifndef WITHOUT_WS
        // * Send changed properties as defined in "4.5 propertyStatus message"
        // Do this by looping over all devices and properties
        ThingDevice *device = this->firstDevice;
        while (device != nullptr)
        {
            sendChangedProperties(device);
            device = device->next;
        }
#endif
    }

    /*
     * Add a thing to the adapter.
     * @param TinyThing* thing
     */
    void addDevice(ThingDevice *device)
    {

        // TODO - I don't fully understand this logic.
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
     * Send a property value to the server.
     * @param TinyProperty property
     */
    void sendChangedProperties(ThingDevice *device)
    {
        // Prepare one buffer per device
        DynamicJsonDocument message(LARGE_JSON_DOCUMENT_SIZE);
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

    void handleThings()
    {
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
        StaticJsonDocument<LARGE_JSON_DOCUMENT_SIZE> doc;
        JsonObject doc2 = doc.to<JsonObject>();
        doc2["messageType"] = "descriptionOfThings";
        doc2["things"] = things;
        String jsonStr;
        serializeJson(doc2, jsonStr);
        sendMessage(jsonStr);
        QA_LOG("[QA:handleThings] Thing description of all devices sent!\n");
    }

    void handleThing(String thingId)
    {

        ThingDevice *device = findDeviceById(thingId);
        if (device == nullptr)
        {
            String msg = "{\"messageType\":\"error\",\"errorCode\":\"404\",\"errorMessage\":\"Thing not found\", \"thingId\": \"" + thingId + "\"}";
            sendMessage(msg);
        }
        DynamicJsonDocument buf(LARGE_JSON_DOCUMENT_SIZE);
        JsonObject descr = buf.to<JsonObject>();
        device->serialize(descr);
        String jsonStr;
        serializeJson(descr, jsonStr);
        sendMessage(jsonStr);

        QA_LOG("[QA:handleThing] Thing description sent!\n");
    }

    void handleThingPropertyGet(String thingId, String propertyId)
    {

        ThingDevice *device = findDeviceById(thingId);
        if (device == nullptr)
        {
            String msg = "{\"messageType\":\"error\",\"errorCode\":\"404\",\"errorMessage\":\"Thing not found\", \"thingId\": \"" + thingId + "\"}";
            sendMessage(msg);
        }
        ThingItem *item = findPropertyById(device, propertyId);
        if (item == nullptr)
        {
            String msg = "{\"messageType\":\"error\",\"errorCode\":\"404\",\"errorMessage\":\"Property not found\", \"thingId\": \"" + thingId + "\", \"propertyId\": \"" + propertyId + "\"}";
            sendMessage(msg);
        }
        DynamicJsonDocument doc(SMALL_JSON_DOCUMENT_SIZE);
        JsonObject prop = doc.to<JsonObject>();
        item->serializeValue(prop);
        DynamicJsonDocument finalDoc(SMALL_JSON_DOCUMENT_SIZE);
        JsonObject finalProp = finalDoc.to<JsonObject>();
        finalDoc["messageType"] = "getProperty";
        finalDoc["thingId"] = thingId;
        finalDoc["properties"] = prop;
        String jsonStr;
        serializeJson(finalProp, jsonStr);
        sendMessage(jsonStr);
    }

    void handleThingPropertiesGet(String thingId)
    {
        ThingItem *rootItem = findDeviceById(thingId)->firstProperty;
        if (rootItem == nullptr)
        {
            String msg = "{\"messageType\":\"error\",\"errorCode\":\"404\",\"errorMessage\":\"Thing not found\", \"thingId\": \"" + thingId + "\"}";
            sendMessage(msg);
        }

        // TODO Instead of using two DyanmicJsonDocument, use one
        DynamicJsonDocument doc(LARGE_JSON_DOCUMENT_SIZE);
        JsonObject prop = doc.to<JsonObject>();
        ThingItem *item = rootItem;
        while (item != nullptr)
        {
            item->serializeValue(prop);
            item = item->next;
        }
        DynamicJsonDocument finalDoc(LARGE_JSON_DOCUMENT_SIZE);
        JsonObject finalProp = finalDoc.to<JsonObject>();
        finalDoc["messageType"] = "getProperty";
        finalDoc["thingId"] = thingId;
        finalDoc["properties"] = prop;
        String jsonStr;
        serializeJson(finalProp, jsonStr);
        sendMessage(jsonStr);
        QA_LOG("[QA:handleThingPropertiesGet] Property data was sent back.\n");
    }

    void handleThingPropertyPut(String thingId, String propertyId, String newPropertyData)
    {

        ThingDevice *device = findDeviceById(thingId);
        if (device == nullptr)
        {
            return;
        }
        ThingProperty *property = findPropertyById(device, propertyId);
        if (property == nullptr)
        {
            return;
        }
        DynamicJsonDocument newBuffer(LARGE_JSON_DOCUMENT_SIZE);
        auto error = deserializeJson(newBuffer, newPropertyData);

        if (error)
        {
            // unable to parse json
            Serial.println("Unable to parse json for property PUT request V2");
            Serial.println(error.c_str());
            // serializeJsonPretty(newPropertyData, Serial);
            return;
        }

        newBuffer["thingId"] = thingId;
        newBuffer["messageType"] = "updatedProperty";
        JsonObject newProp = newBuffer.as<JsonObject>();
        device->setProperty(property->id.c_str(), newProp["value"]);
        String jsonStr;
        serializeJson(newProp, jsonStr);
        sendMessage(jsonStr);

        QA_LOG("[QA:handleThingPropertyPut] Property value has been set! \n");
    }
};