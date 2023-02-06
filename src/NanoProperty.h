#pragma once

#include <Arduino.h>
#include "pb_helper.h"

enum ThingDataType
{
    NO_STATE,
    BOOLEAN,
    NUMBER,
    INTEGER,
    STRING
};
typedef enum ThingDataType thingDataType;

union  ThingDataValue
{
    bool boolean;
    double number;
    signed long long integer;
    String *string;
};
typedef union ThingDataValue thingDataValue;

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
        * @brief Get the value of the property
        * @return TinyDataValue value : {boolean, number, integer, string}
        */
    ThingDataValue getValue() { return this->value; }


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

    void serialize(Property *property)
    {
        // Todo: serialize the value
    }

    void serializeValue() {
        // Todo: serialize the value
    }


    private:
    ThingDataValue value = {0};
    bool hasChanged = false;
};

class ThingProperty : public ThingItem
{
    private:
        void (*callback)(ThingPropertyValue);
    
    public:
        ThingProperty(const char *id_, ThingDataType type_,
                  const char *atType_,
                  void (*callback_)(ThingPropertyValue) = nullptr)
        : ThingItem(id_, type_, atType_), callback(callback_) {}

    void serialize(Property *property)
    {}

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


}
