#include "TinyProperty.h"

TinyItem::TinyItem(const char *id_, TinyDataType type_, const char *contextType_) {
        this->id = id_;
        this->type = type_;
        this->contextType = contextType_;
        this->value = {false};
        this->hasChanged = false;
    }

void TinyItem::setValue(TinyDataValue value)
{
    this->value = value;
    this->hasChanged = true;
}

TinyDataValue TinyItem::getValue()
{
    return this->value;
}

TinyDataValue TinyItem::*getChangedValueOrNull()
{
    TinyDataValue *result = this->hasChanged ? &this->value : nullptr;
    this->hasChanged = false;
    return result;
}

void TinyItem::serialize(JsonObject obj, String thingId, String resourceType)
{

    switch (type)
    {
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

    if (unit.length() > 0)
    {
        obj["unit"] = unit;
    }

    if (contextType.length() > 0)
    {
        obj["@type"] = contextType;
    }

    obj["href"] = "/things/" + thingId + "/" + resourceType + "/" + id;
}

void TinyItem::serializeValue(JsonObject prop)
{
    switch (this->type)
    {
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
        prop[this->id] = this->getValue().string->c_str();
        break;
    }
}



void TinyProperty::serialize(JsonObject obj, String thingId, String resourceType)
{
    TinyItem::serialize(obj, thingId, resourceType);
}

void TinyProperty::changed(TinyPropertyValue value)
{
    if (callback != nullptr)
    {
        callback(value);
    }
}