#pragma once

#define ARDUINOJSON_USE_LONG_LONG 1
#include <ArduinoJson.h>

#define LARGE_JSON_DOCUMENT_SIZE 4096
#define SMALL_JSON_DOCUMENT_SIZE 1024

enum ThingDataType
{
    NO_STATE,
    BOOLEAN,
    NUMBER,
    INTEGER,
    STRING
};
typedef ThingDataType ThingPropertyType;

/*
 * This is a union of all the possible data types that can be stored in a TinyItem.
 * Only one of these will be used at a time.
 */
union ThingDataValue
{
    bool boolean;
    double number;
    signed long long integer;
    String *string;
};
typedef ThingDataValue ThingPropertyValue;

/*
 * This is the base class for all properties.
 */
class ThingItem
{
public:
    String id;
    ThingDataType type;
    String atType;
    ThingItem *next = nullptr;
    String unit = "";

    ThingItem(const char *id_, ThingDataType type_,
              const char *atType_)
        : id(id_), type(type_), atType(atType_) {}

    /*
     * @brief Set the value of the property
     * @param TinyDataValue value
     */
    void setValue(ThingDataValue newValue)
    {
        this->value = newValue;
        this->hasChanged = true;
    }

    /*
     * @brief Set the value of the property
     * @param const char *s
     */
    void setValue(const char *s)
    {
        *(this->getValue().string) = s;
        this->hasChanged = true;
    }

    /*
     * @brief Get changed value of the property, if any (and reset it)
     * @return TinyDataValue : {boolean, number, integer, string} or NULL
     */
    ThingDataValue *changedValueOrNull()
    {
        ThingDataValue *v = this->hasChanged ? &this->value : nullptr;
        this->hasChanged = false;
        return v;
    }

    /*
     * @brief Get the value of the property
     * @return TinyDataValue : {boolean, number, integer, string}
     */
    ThingDataValue getValue() { return this->value; }

        /*
     * @brief Serialize the property to JSON
     * @param JsonObject &json
     * @param String thingId
     * @param String resourceType
     * @example
     * {
     *    "id": "temperature",
     *    "type": "number",
     *    "unit": "degree celsius",
     *    "href": "/things/lamp/properties/temperature"
     * }
     */
    void serialize(JsonObject obj, String deviceId)
    {
        switch (type)
        {
        case NO_STATE:
            break;
        case BOOLEAN:
            obj["type"] = "boolean";
            break;
        case NUMBER:
            obj["type"] = "number";
            break;
        case INTEGER:
            obj["type"] = "integer";
            break;
        case STRING:
            obj["type"] = "string";
            break;
        }

        if (unit != "")
        {
            obj["unit"] = unit;
        }

        if (atType != nullptr)
        {
            obj["@contextType"] = atType;
        }
    }

    /*
     * @brief Serialize the value of the property to JSON
     * @param JsonObject &json
     * @example
     * {
     *  "on": false
     * }
     */
    void serializeValue(JsonObject prop)
    {
        switch (this->type)
        {
        case NO_STATE:
            break;
        case BOOLEAN:
            prop[this->id] = this->getValue().boolean;
            break;
        case NUMBER:
            prop[this->id] = this->getValue().number;
            break;
        case INTEGER:
            prop[this->id] = this->getValue().integer;
            break;
        case STRING:
            prop[this->id] = *this->getValue().string;
            break;
        }
    }

private:
    /* @brief stores the current state of the property */
    ThingDataValue value = {false};
    /* @brief stores if the property has changed since last read */
    bool hasChanged = false;
};

class ThingProperty : public ThingItem
{
private:
    void (*callback)(ThingPropertyValue);

public:
    const char **propertyEnum = nullptr;

    ThingProperty(const char *id_, ThingDataType type_,
                  const char *atType_,
                  void (*callback_)(ThingPropertyValue) = nullptr)
        : ThingItem(id_, type_, atType_), callback(callback_) {}

    /*
     * @brief Serialize the property to JSON
     * @param JsonObject obj
     * @param String thingId
     * @param String resourceType
     */
    void serialize(JsonObject obj, String deviceId)
    {
        ThingItem::serialize(obj, deviceId);

        const char **enumVal = propertyEnum;
        bool hasEnum = propertyEnum != nullptr && *propertyEnum != nullptr;

        if (hasEnum)
        {
            enumVal = propertyEnum;
            JsonArray propEnum = obj.createNestedArray("enum");
            while (propertyEnum != nullptr && *enumVal != nullptr)
            {
                propEnum.add(*enumVal);
                enumVal++;
            }
        }
    }

    /*
     * @brief If the property has changed, call the callback function
     * if it exists.
     * @param TinyDataValue value
     */
    void changed(ThingPropertyValue newValue)
    {
        if (callback != nullptr)
        {
            callback(newValue);
        }
    }
};

class ThingDevice
{
public:
    String id;
    ThingDevice *next = nullptr;
    ThingProperty *firstProperty = nullptr;

    ThingDevice(const char *_id)
        : id(_id) {}

    /*
     * @brief Find a property with the given id
     * @param TinyProperty *property
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
     * @param TinyProperty *property
     */
    void addProperty(ThingProperty *property)
    {
        property->next = firstProperty;
        firstProperty = property;
    }

     /*
     * @brief Set a property value
     * @param {String} id
     * @param {JsonVariant} newValue
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
     * @param {JsonObject} descr
     * @param {String} tunnelUrl
     */
    void serialize(JsonObject descr)
    {
        descr["id"] = this->id;
        ThingProperty *property = this->firstProperty;
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
