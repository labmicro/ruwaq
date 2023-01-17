/************************************************************************************************
Copyright (c) 2022-2023, Laboratorio de Microprocesadores
Facultad de Ciencias Exactas y Tecnología, Universidad Nacional de Tucumán
https://www.microprocesadores.unt.edu.ar/

Copyright (c) 2022-2023, Esteban Volentini <evolentini@herrera.unt.edu.ar>

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
associated documentation files (the "Software"), to deal in the Software without restriction,
including without limitation the rights to use, copy, modify, merge, publish, distribute,
sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial
portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES
OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

SPDX-License-Identifier: MIT
*************************************************************************************************/

/** @file
 ** @brief Remote execution protocol implementation
 **
 ** @addtogroup preat PREAT
 ** @brief Protocol for Remote Excecution of Automated Tests
 ** @{ */

/* === Headers files inclusions =============================================================== */

#include "protocol.h"
#include "crc.h"
#include "assertion.h"
#include <string.h>

/* === Macros definitions ====================================================================== */

#define HANDLERS_POOL_SIZE 128

#define ID_NOT_FOUND       0xFFFF

#define InternalsCount()   sizeof(internals) / sizeof(struct handler_descriptor_s)

/* === Private data type declarations ========================================================== */

typedef struct preat_message_s {
    uint16_t method;
    struct preat_parameter_s parameters[16];
    uint8_t param_count;
} * preat_message_t;

typedef struct handler_descriptor_s {
    bool output : 1;
    uint16_t id : 15;
    preat_method_t handler;
    preat_type_t const * parameters;
} const * handler_descriptor_t;

typedef struct handlers_pool_s {
    uint16_t next_free;
    struct handler_descriptor_s pool[HANDLERS_POOL_SIZE];
} * handlers_pool_t;

typedef struct waiting_assertion_s {
    uint32_t start;
    uint32_t stop;
    uint8_t conditions;
    struct preat_message_s methods[8];
    bool active;
} * waiting_assertion_t;

/* === Private variable declarations =========================================================== */

/* === Private function declarations =========================================================== */

/* === Public variable definitions ============================================================= */

const preat_type_t SINGLE_UINT8_PARAM[] = {TYPE_UINT8, TYPE_UNDEFINED};

const preat_type_t WAIT_ASSERT_PARAM[] = {TYPE_UINT32, TYPE_UINT32, TYPE_UINT8, TYPE_UINT8,
                                          TYPE_UNDEFINED};

/* === Private variable definitions ============================================================ */

static struct handlers_pool_s handlers = {0};

static const struct handler_descriptor_s internals[] = {
    {.id = 0x005, .handler = AssertStart, .parameters = WAIT_ASSERT_PARAM},
};

/* === Private function implementation ========================================================= */

static handler_descriptor_t FindDescriptor(uint16_t id) {
    handler_descriptor_t result = NULL;
    for (uint16_t index = 0; index < InternalsCount(); index++) {
        if (internals[index].id == id) {
            result = &internals[index];
            break;
        }
    }
    if (result == NULL) {
        for (uint16_t index = 0; index < handlers.next_free; index++) {
            if (handlers.pool[index].id == id) {
                result = &(handlers.pool[index]);
                break;
            }
        }
    }
    return result;
}

static preat_error_t DecodeFrame(uint8_t * frame, preat_message_t message) {
    uint8_t index, type;
    preat_parameter_t parameter;
    crc_t crc;

    crc = crc_init();
    crc = crc_update(crc, frame, frame[0]);
    crc = crc_finalize(crc);

    if (crc) {
        return PREAT_CRC_ERROR;
    }

    message->method = ((uint16_t)frame[1] << 4) | (frame[2] >> 4);
    memset(message->parameters, 0, sizeof(message->parameters));

    message->param_count = frame[2] & 0x0F;
    frame = frame + 3;
    parameter = message->parameters;

    for (index = 0; index < message->param_count; index++) {
        if ((index & 0x01) == 0) {
            type = frame[0];
            frame = frame + 1;
        } else {
            type = type << 4;
        }
        switch (type >> 4) {
        case 0x01:
            parameter->type = TYPE_UINT8;
            parameter->value = frame[0];
            frame = frame + 1;
            break;
        case 0x02:
            parameter->type = TYPE_UINT16;
            parameter->value = frame[0];
            parameter->value <<= 8;
            parameter->value |= frame[1];
            frame = frame + 2;
            break;
        case 0x03:
            parameter->type = TYPE_UINT32;
            parameter->value = frame[0];
            parameter->value <<= 8;
            parameter->value |= frame[1];
            parameter->value <<= 8;
            parameter->value |= frame[2];
            parameter->value <<= 8;
            parameter->value |= frame[3];
            frame = frame + 4;
            break;
        }
        parameter = parameter + 1;
    }

    return PREAT_NO_ERROR;
}

static bool CompareParameters(preat_message_t message, handler_descriptor_t descriptor) {
    preat_type_t const * declared = descriptor->parameters;
    preat_parameter_t received = message->parameters;
    bool result = (*declared == received->type);

    while ((declared != TYPE_UNDEFINED) && (received->type != TYPE_UNDEFINED)) {
        result = result && (*declared == received->type);
        declared++;
        received++;
    }
    return result;
}

static void EncodeResponse(uint8_t * frame, preat_error_t result) {
    static const uint8_t ACK[] = {0x05, 0x00, 0x00, 0xa1, 0xb5};
    static const uint8_t NACK[] = {0x07, 0x00, 0x11, 0x10, 0x00, 0x00, 0x00};
    crc_t crc;

    if (result == PREAT_NO_ERROR) {
        memcpy(frame, ACK, sizeof(ACK));
    } else {
        memcpy(frame, NACK, sizeof(NACK));
        frame[4] = (uint8_t)result;

        crc = crc_init();
        crc = crc_update(crc, frame, sizeof(NACK) - 2);
        crc = crc_finalize(crc);

        frame[5] = (uint8_t)(crc >> 8);
        frame[6] = (uint8_t)(crc & 0xFF);
    }
}

/* === Public function implementation ========================================================== */

bool PreatRegister(uint16_t id, bool output, preat_method_t handler,
                   preat_type_t const * parameters) {
    struct handler_descriptor_s * descriptor = NULL;

    if (handlers.next_free < HANDLERS_POOL_SIZE) {
        descriptor = &(handlers.pool[handlers.next_free]);
        handlers.next_free++;
    }
    if (descriptor) {
        descriptor->id = id;
        descriptor->output = output;
        descriptor->handler = handler;
        descriptor->parameters = parameters;
    }

    return (descriptor != NULL);
}

void PreatExecute(uint8_t * frame) {
    static struct preat_message_s message;
    handler_descriptor_t descriptor = NULL;
    preat_error_t result;

    result = DecodeFrame(frame, &message);
    if (result == PREAT_NO_ERROR) {
        descriptor = FindDescriptor(message.method);
        if (descriptor == NULL) {
            result = PREAT_METHOD_ERROR;
        }
    }

    if (result == PREAT_NO_ERROR) {
        if (CompareParameters(&message, descriptor)) {
            if ((descriptor->output) && (AssertIsDefined())) {
                result =
                    AssertExecute(descriptor->handler, message.parameters, message.param_count);
            } else {
                result = descriptor->handler(message.parameters, message.param_count);
            }
        } else {
            result = PREAT_PARAMETERS_ERROR;
        }
    }

    EncodeResponse(frame, result);
}

/* === End of documentation ==================================================================== */

/** @} End of module definition for doxygen */
