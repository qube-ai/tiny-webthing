#pragma once

#include "thing.pb.h"
#include "pb_common.h"
#include "pb_encode.h"
#include <Arduino.h>
#include "config.h"

/*
 * Encodes a string into a buffer.
 */
bool encode_string(pb_ostream_t *stream, const pb_field_t *field, void * const *arg)
{
    if (!pb_encode_tag_for_field(stream, field))
        return false;
    
    return pb_encode_string(stream, (uint8_t *)*arg, strlen((char *)*arg));
}


/*
 * Encodes a repeated submessage into a buffer.
 */
bool encode_repeated_messages(pb_ostream_t *stream, const pb_field_t *field, void * const *arg) {
    Property *properties = (Property *)*arg;
    for (int i = 0; i < NUMBER_OF_PROPERTIES; i++) {
        if (!pb_encode_tag_for_field(stream, field))
            return false;
        if (!pb_encode_submessage(stream, Property_fields, &properties[i]))
            return false;
    }
    return true;
}


