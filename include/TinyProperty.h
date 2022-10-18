#pragma once

#ifndef TinyProperty_h
#define TinyProperty_h

#include <Arduino.h>
#include <ArduinoJson.h>

enum TinyDataType
{
    BOOLEAN,
    NUMBER,
    INTEGER,
    STRING
};
typedef TinyDataType TinyDataType;

/*
 * This is a union of all the possible data types that can be stored in a TinyItem.
 * Only one of these will be used at a time.
 */
union TinyDataValue
{
    bool boolean;
    double number;
    signed long long integer;
    String *string;
};
typedef TinyDataValue TinyPropertyValue;

/*
 * This is the base class for all properties.
 */
class TinyItem
{

public:
    String id;
    TinyDataType type;
    String contextType;
    TinyItem *nextItem = nullptr;
    String unit = "";
    TinyItem(const char *id_, TinyDataType type_, const char *contextType_);

    /*
     * @brief Set the value of the property
     * @param TinyDataValue value
     */
    void setValue(TinyDataValue value);

    /*
     * @brief Get the value of the property
     * @return TinyDataValue : {boolean, number, integer, string}
     */
    TinyDataValue getValue();

    /*
     * @brief Get changed value of the property, if any (and reset it)
     * @return TinyDataValue : {boolean, number, integer, string} or NULL
     */
    TinyDataValue *getChangedValueOrNull();

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
    void serialize(JsonObject obj, String thingId, String resourceType);

    /*
     * @brief Serialize the value of the property to JSON
     * @param JsonObject &json
     * @example
     * {
     *  "on": false
     * }
     */
    void serializeValue(JsonObject prop);

private:
    TinyDataValue value = {false};
    bool hasChanged = false;
};

class TinyProperty : public TinyItem
{
private:
    void (*callback)(TinyDataValue) = nullptr;

public:
    TinyProperty(const char *id_, TinyDataType type_,
                 const char *contextType_,
                 void (*callback_)(TinyPropertyValue) = nullptr)
        : TinyItem(id_, type_, contextType_), callback(callback_) {}

    /*
     * @brief Serialize the property to JSON
     * @param JsonObject obj
     * @param String thingId
     * @param String resourceType
     */
    void serialize(JsonObject obj, String thingId, String resourceType);

    /*
     * @brief 
     */
    void changed(TinyPropertyValue value);
};

#endif