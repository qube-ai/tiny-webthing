#include "TinyThing.h"


TinyThing::TinyThing(const char *thingId_) {
    this->thingId = thingId_;
    this->next = nullptr;
    this->firstProperty = nullptr;
}

TinyProperty TinyThing::*findProperty(const char *id)
{
    TinyProperty *property = this->firstProperty;
    while (property)
    {
        if (!strcmp(property->id.c_str(), id))
        {
            return property;
        }
        property = (TinyProperty *)property->next;
    }
    return nullptr;
}

void TinyThing::addProperty(TinyProperty *property)
{
    property->next = firstProperty;
    firstProperty = property;
}

void TinyThing::setProperty(const char *id, const JsonVariant &newValue)
{
    TinyProperty *property = TinyThing::findProperty(id);
    if (property == nullptr)
    {
        return;
    }

    switch (property->type)
    {
    case BOOLEAN:
    {
        TinyDataValue value;
        value.boolean = newValue.as<bool>();
        property->setValue(value);
        property->changed(value);
        break;
    }
    case NUMBER:
    {
        TinyDataValue value;
        value.number = newValue.as<double>();
        property->setValue(value);
        property->changed(value);
        break;
    }
    case INTEGER:
    {
        TinyDataValue value;
        value.integer = newValue.as<signed long long>();
        property->setValue(value);
        property->changed(value);
        break;
    }
    case STRING:
    {
        TinyDataValue value;
        value.string = new String(newValue.as<String>());
        property->setValue(value);
        property->changed(value);
        break;
    }
    }
}

void TinyThing::serialize(JsonObject descr, String tunnelUrl)
{
    descr["id"] = this->thingId;
    descr["base"] = tunnelUrl + "/things/" + this->thingId;
    // Create wss url from http url
    tunnelUrl.replace("https", "wss");
    descr["wsUrl"] = tunnelUrl + "/things/" + this->thingId + "/ws";

    TinyProperty *property = this->firstProperty;
    if (property != nullptr)
    {
        JsonObject properties = descr.createNestedObject("properties");
        while (property != nullptr)
        {
            JsonObject obj = properties.createNestedObject(property->id);
            property->serialize(obj, this->thingId, "properties");
            property = (TinyProperty *)property->next;
        }
    }
}