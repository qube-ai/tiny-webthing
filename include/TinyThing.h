

#ifndef TinyThing_h
#define TinyThing_h

#include "TinyProperty.h"

#include <Arduino.h>
#include <ArduinoJson.h>

class TinyThing
{
public:
    String thingId;
    TinyThing *next = nullptr;
    TinyProperty *firstProperty = nullptr;

    TinyThing(const char *thingId_)
        : thingId(thingId_) {}

    /*
     * @brief Find a property with the given id
     * @param TinyProperty *property
     * @return TinyProperty *property
     */
    TinyProperty *findProperty(const char *id);

    /*
     * @brief Add a property to the thing
     * @param TinyProperty *property
     */
    void addProperty(TinyProperty *property);

    /*
     * @brief Set a property value
     * @param {String} id
     * @param {JsonVariant} newValue
     */
    void setProperty(const char *id, const JsonVariant &newValue);

    /*
     * @brief Serialize the thing to JSON
     * @param {JsonObject} descr
     * @param {String} tunnelUrl
     */
    void serialize(JsonObject descr, String tunnelUrl);
};

#endif