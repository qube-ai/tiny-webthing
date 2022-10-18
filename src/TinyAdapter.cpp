#include "TinyAdapter.h"


TinyAdapter::TinyAdapter(const char *_tunnelUrl) {
    tunnelUrl = _tunnelUrl;
    firstThing = nullptr;
    lastThing = nullptr;
}

void TinyAdapter::sendMessage(String &msg)
{
    webSocket.sendTXT(msg.c_str(), msg.length() + 1);
}

void TinyAdapter::messageHandler(String payload)
{

    DynamicJsonDocument doc(SMALL_JSON_OBJECT);
    DeserializationError error = deserializeJson(doc, payload);
    if (error)
    {
        TA_LOG("[TA.mh] deserializeJson() failed: %s \n", error.c_str());
        String msg = "{\"messageType\":\"error\", \"errorMessage\":\"deserializeJson() failed \"}";
        sendMessage(msg);
    }

    JsonObject root = doc.as<JsonObject>();

    if (root["messageType"] == "getProperty")
    {
        TA_LOG("[TA.mh] getProperty() request received\n");
        String thingId = root["thingId"];
        getProperties(thingId);
    }

    else if (root["messageType"] == "setProperty")
    {
        TA_LOG("[TA.mh] setProperty() request received\n");
        String thingId = root["thingId"];
        String propertyId = root["data"]["propertyId"];
        String data = root["data"];
        setProperty(thingId, propertyId, data);
        TA_LOG("[TA.mh] setProperty() request processed\n");
    }

    else if (root["messageType"] == "getThingDescription")
    {
        TA_LOG("[TA.mh] getThingDescription() request received\n");
        String thingId = root["thingId"];
        getThingDescription();
        TA_LOG("[TA.mh] getThingDescription() request processed\n");
    }

    else if (root["messageType"] == "getAllThings")
    {
        TA_LOG("[TA.mh] getAllThings() request received\n");
        getThingDescription();
        TA_LOG("[TA.mh] getAllThings() request processed\n");
    }

    else
    {
        TA_LOG("[TA.mh] Unknown messageType");
        TA_LOG("[TA.mh] messageType: %s \n", root["messageType"]);
    }
}

void TinyAdapter::onWebSocketEvent(WStype_t type, uint8_t *payload, size_t length)
{
    switch (type)
    {
    case WStype_DISCONNECTED:
        TA_LOG("[TA:webSocketEvent] Disconnect!\n");
        break;

    case WStype_CONNECTED:
        TA_LOG("[TA:webSocketEvent] Connected to server. Payload ->: %s\n", payload);
        webSocket.sendTXT("{\"messageType\":\"StartWs\"}");
        break;

    case WStype_TEXT:
    {
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

    case WStype_PING:
        TA_LOG("[TA:webSocketEvent] Ping!\n");
        break;

    case WStype_PONG:
        TA_LOG("[TA:webSocketEvent] Pong!\n");
        break;
    }
}

TinyThing* TinyAdapter::findThingById(String id)
{
    TinyThing *thing = firstThing;
    while (thing != nullptr)
    {
        if (thing->thingId == id)
        {
            return thing;
        }
        thing = thing->nextThing;
    }
    return nullptr;
}

TinyProperty* TinyAdapter::findPropertyById(TinyThing *thing, String id)
{
    TinyProperty *property = thing->firstProperty;
    while (property != nullptr)
    {
        if (property->id == id)
        {
            return property;
        }
        property = (TinyProperty *)property->nextItem;
    }
    return nullptr;
}

void TinyAdapter::begin(String websocketUrl, int websocketPort, String websocketPath)
{
    webSocket.begin(websocketUrl, websocketPort, websocketPath);
    webSocket.onEvent(std::bind(
        &TinyAdapter::onWebSocketEvent, this, std::placeholders::_1,
        std::placeholders::_2, std::placeholders::_3));
}

void TinyAdapter::beginSSL(String websocketUrl, int websocketPort, String websocketPath)
{
    webSocket.beginSSL(websocketUrl.c_str(), websocketPort, websocketPath.c_str());
    webSocket.onEvent(std::bind(
        &TinyAdapter::onWebSocketEvent, this, std::placeholders::_1,
        std::placeholders::_2, std::placeholders::_3));
}

void TinyAdapter::update()
{
    TA_LOG("[TA.update] updating connection loop.... \n");
    webSocket.loop();
    TA_LOG("[TA.update] connection loop updated \n");
    TinyThing *device = firstThing;
    while (device != nullptr)
    {
        sendChangedProperties(device);
        device = device->nextThing;
    }
    TA_LOG("[TA:update] updateThings() finished \n");
}

void TinyAdapter::addThing(TinyThing *thing)
{
    if (firstThing == nullptr)
    {
        firstThing = thing;
        lastThing = thing;
    }
    else
    {
        lastThing->nextThing = thing;
        lastThing = thing;
    }
}

void TinyAdapter::sendChangedProperties(TinyThing *thing)
{
    DynamicJsonDocument message(SMALL_JSON_OBJECT);
    message["messageType"] = "propertyStatus";
    JsonObject prop = message.createNestedObject("data");
    bool dataToSend = false;
    TinyItem *item = thing->firstProperty;
    while (item != nullptr)
    {
        TinyDataValue *value = item->getChangedValueOrNull();
        if (value)
        {
            dataToSend = true;
            item->serializeValue(prop);
        }
        item = item->nextItem;
    }
    if (dataToSend)
    {
        String jsonStr;
        message["thingId"] = thing->thingId;
        serializeJson(message, jsonStr);
        sendMessage(jsonStr);
    }
}

void TinyAdapter::getThingDescription()
{
    DynamicJsonDocument buf(LARGE_JSON_OBJECT);
    JsonArray things = buf.to<JsonArray>();
    TinyThing *thing = firstThing;
    while (thing != nullptr)
    {
        JsonObject descr = things.createNestedObject();
        thing->serialize(descr, tunnelUrl);
        descr["href"] = "/things/" + thing->thingId;
        thing = thing->nextThing;
    }
    DynamicJsonDocument doc(LARGE_JSON_OBJECT);
    JsonObject doc2 = doc.to<JsonObject>();
    doc2["messageType"] = "descriptionOfThings";
    doc2["things"] = things;
    String jsonStr;
    serializeJson(doc2, jsonStr);
    sendMessage(jsonStr);
}

void TinyAdapter::getProperties(String thingId)
{
    TinyItem *rootItem = findThingById(thingId)->firstProperty;
    if (rootItem == nullptr)
    {
        return;
    }
    DynamicJsonDocument doc(SMALL_JSON_OBJECT);
    JsonObject prop = doc.to<JsonObject>();
    TinyItem *item = rootItem;
    while (item != nullptr)
    {
        item->serializeValue(prop);
        item = item->nextItem;
    }
    DynamicJsonDocument finalDoc(SMALL_JSON_OBJECT);
    JsonObject finalProp = finalDoc.to<JsonObject>();
    finalDoc["messageType"] = "getProperty";
    finalDoc["thingId"] = thingId;
    finalDoc["properties"] = prop;
    String jsonStr;
    serializeJson(finalProp, jsonStr);
    sendMessage(jsonStr);
}

void TinyAdapter::setProperty(String thingId, String propertyId, String value)
{
    TinyThing *thing = findThingById(thingId);
    if (thing == nullptr)
    {
        return;
    }
    TinyProperty *property = findPropertyById(thing, propertyId);
    if (property == nullptr)
    {
        return;
    }
    DynamicJsonDocument newBuffer(LARGE_JSON_OBJECT);
    auto error = deserializeJson(newBuffer, value);

    if (error)
    {
        Serial.println("Unable to parse json for property PUT request V2");
        Serial.println(error.c_str());
        return;
    }

    newBuffer["thingId"] = thingId;
    newBuffer["messageType"] = "updatedProperty";
    JsonObject newProp = newBuffer.as<JsonObject>();
    thing->setProperty(property->id.c_str(), newProp["value"]);
    String jsonStr;
    serializeJson(newProp, jsonStr);
    sendMessage(jsonStr);
}