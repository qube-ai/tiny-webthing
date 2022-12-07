#pragma once

#define ARDUINOJSON_USE_LONG_LONG 1
#include <ArduinoJson.h>
#include "ThingProperty.h"

class ThingDevice
{
public:
    String id;
    const char **type;
    ThingDevice *next = nullptr;
    ThingProperty *firstProperty = nullptr;

    ThingDevice(const char *_id, const char **_type)
        : id(_id), type(_type) {}

    /*
     * @brief Find a property with the given id
     * @param  {String} id : property id
     * @return TinyProperty *property
     */
    ThingProperty *findProperty(const char *id)
    {
        ThingProperty *p = this->firstProperty;
        while (p)
        {
            if (!strcmp(p->id.c_str(), id))
                return p;
            p = (ThingProperty *)p->next;
        }
        return nullptr;
    }

    /*
     * @brief Add a property to the thing
     * @param {TinyProperty} property
     */
    void addProperty(ThingProperty *property)
    {
        property->next = firstProperty;
        firstProperty = property;
    }

    /*
     * @brief Set a property value
     * @param {String} id : property id
     * @param {JsonVariant} newValue : new value of the property (boolean, number, integer, string)
     */
    void setProperty(const char *name, const JsonVariant &newValue)
    {
        ThingProperty *property = findProperty(name);

        if (property == nullptr)
        {
            return;
        }

        switch (property->type)
        {
        case NO_STATE:
        {
            break;
        }
        case BOOLEAN:
        {
            ThingDataValue value;
            value.boolean = newValue.as<bool>();
            property->setValue(value);
            property->changed(value);
            break;
        }
        case NUMBER:
        {
            ThingDataValue value;
            value.number = newValue.as<double>();
            property->setValue(value);
            property->changed(value);
            break;
        }
        case INTEGER:
        {
            ThingDataValue value;
            value.integer = newValue.as<signed long long>();
            property->setValue(value);
            property->changed(value);
            break;
        }
        case STRING:
            property->setValue(newValue.as<const char *>());
            property->changed(property->getValue());
            break;
        }
    }

    /*
     * @brief Serialize the thing to JSON
     * @param {JsonObject} descr : JSON object to serialize to
     */
    void serialize(JsonObject descr)
    {
        descr["id"] = this->id;
        ThingProperty *property = this->firstProperty;
        descr["@context"] = "https://webthings.io/schemas";

        JsonArray typeJson = descr.createNestedArray("@type");
        const char **type = this->type;
        while ((*type) != nullptr)
        {
            typeJson.add(*type);
            type++;
        }

        if (property != nullptr)
        {
            JsonObject properties = descr.createNestedObject("properties");
            while (property != nullptr)
            {
                JsonObject obj = properties.createNestedObject(property->id);
                property->serialize(obj, id);
                property = (ThingProperty *)property->next;
            }
        }
    }
};
