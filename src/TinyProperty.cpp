#include "TinyProperty.h"

TinyItem::TinyItem(const char *id_, TinyDataType type_, const char *contextType_)
{
    id = id_;
    type = type_;
    contextType = contextType_;
    value = {false};
    hasChanged = false;
}

void TinyItem::setValue(TinyDataValue value)
{
    value = value;
    hasChanged = true;
}

TinyDataValue TinyItem::getValue()
{
    return value;
}

TinyDataValue* TinyItem::getChangedValueOrNull()
{
    TinyDataValue *result =  hasChanged ? &value : nullptr;
    hasChanged = false;
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
    switch (type)
    {
    case BOOLEAN:
        prop[id] = TinyItem::getValue().boolean;
        break;

    case NUMBER:
        prop[id] = TinyItem::getValue().number;
        break;

    case INTEGER:
        prop[id] = TinyItem::getValue().integer;
        break;

    case STRING:
        prop[id] = TinyItem::getValue().string->c_str();
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