#pragma once

#include <ArduinoJson.h>

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
     * @param TinyDataValue value : {boolean, number, integer, string}
     */
    void setValue(ThingDataValue newValue)
    {
        this->value = newValue;
        this->hasChanged = true;
    }

    /*
     * @brief Set the value of the property
     * @param const char *s : string value
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
     * @param String deviceId
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
            obj["@type"] = atType;
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
     * @param JsonObject obj : JSON object to serialize to
     * @param String deviceId : ID of the thing
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
     * @param TinyDataValue value : {boolean, number, integer, string}
     */
    void changed(ThingPropertyValue newValue)
    {
        if (callback != nullptr)
        {
            callback(newValue);
        }
    }
};
