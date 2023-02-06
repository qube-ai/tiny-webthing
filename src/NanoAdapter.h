#pragma once

#include <Arduino.h>
#include "Protocol.h"
#include "pb_helper.h"
#include "NanoThing.h"


class NanoAdapter
{

    public:
        NanoAdapter(String _websocketUrl, int _port, String _websocketPath)
        : websocketUrl(_websocketUrl),  port(_port), websocketPath(_websocketPath) {}

    String websocketUrl;   // "eg: ws://<somehost>"
    int port; // port to connect to
    String websocketPath; // "eg: /ws"

    ThingDevice *firstDevice = nullptr;
    ThingDevice *lastDevice = nullptr;
    ProtocolHandler protocolHandler(websocketUrl, port, websocketPath, onTextMessage, onBinaryMessage);

    void (*onTextMessage)(uint8_t *payload, size_t length);
    void (*onBinaryMessage)(uint8_t *payload, size_t length);

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

    void begin(bool secure) {
        WebsocketConfig config;
        config.websocketPort = port;
        config.websocketUrl = websocketUrl;
        config.websocketPath = websocketPath;
        if (secure) {
            protocol.beginSecure(websocketUrl, port, websocketPath);
        } else {
            protocol.begin(websocketUrl, port, websocketPath);
        }
    }

    void update() {
        protocol.update();
    }

    void sendChangedProperties(ThingDevice) {
        
    }

}