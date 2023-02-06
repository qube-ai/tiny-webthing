#pragma once

#include <Arduino.h>
#include "pb_helper.h"
#include "NanoProperty.h"

class ThingDevice
{
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

    void setProperty() {}
    void serialize() {}
}
