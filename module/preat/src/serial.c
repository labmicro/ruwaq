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
 ** @brief Serial server for preat protocol implementation
 **
 ** @addtogroup preat PREAT
 ** @brief Protocol for Remote Excecution of Automated Tests
 ** @{ */

/* === Headers files inclusions =============================================================== */

#include "serial.h"
#include "protocol.h"
#include <string.h>

/* === Macros definitions ====================================================================== */

/* === Private data type declarations ========================================================== */

typedef struct reception_buffer_s {
    uint16_t received;
    uint8_t data[64];
} * reception_buffer_t;

typedef struct transmission_buffer_s {
    uint16_t transmited;
    uint8_t data[64];
} * transmission_buffer_t;

struct preat_server_s {
    hal_sci_t sci;
    preat_event_t handler;
    void * object;
    struct reception_buffer_s rxd[1];
    struct transmission_buffer_s txd[1];
};

/* === Private variable declarations =========================================================== */

/* === Private function declarations =========================================================== */

/* === Public variable definitions ============================================================= */

/* === Private variable definitions ============================================================ */

static struct preat_server_s server_instances[1];

/* === Private function implementation ========================================================= */

static void SerialEvent(hal_sci_t sci, sci_status_t status, void * object) {
    preat_server_t server = object;
    uint16_t length;
    uint8_t * data;

    if (status->data_ready) {
        data = server->rxd->data + server->rxd->received;
        if (server->rxd->received == 0) {
            length = sizeof(server->rxd->data);
        } else {
            length = server->rxd->data[0] - server->rxd->received;
        }
        server->rxd->received += SciReceiveData(sci, data, length);
        if (server->rxd->data[0] == server->rxd->received) {
            if (server->handler) {
                server->handler(server, server->object);
            }
        }
    }
    if ((status->fifo_empty) & (server->txd->data[0] != 0)) {
        data = server->txd->data + server->txd->transmited;
        length = server->txd->data[0] - server->txd->transmited;
        server->txd->transmited += SciSendData(sci, data, length);
        if (server->txd->data[0] == server->txd->transmited) {
            server->txd->data[0] = 0;
            server->txd->transmited = 0;
        }
    }
}

/* === Public function implementation ========================================================== */

preat_server_t ServerStartSerial(hal_sci_t sci, hal_sci_pins_t serial_pins) {
    static const struct hal_sci_line_s port_config = {
        .baud_rate = 115200,
        .data_bits = 8,
        .parity = HAL_SCI_NO_PARITY,
    };
    preat_server_t server = NULL;

    if (SciSetConfig(sci, &port_config, serial_pins)) {
        server = server_instances;

        memset(server, 0, sizeof(struct preat_server_s));
        server->sci = sci;
        SciSetEventHandler(sci, SerialEvent, server);
    }
    return server;
}

void ServerSetEventHandler(preat_server_t server, preat_event_t handler, void * object) {
    if (server) {
        server->handler = handler;
        server->object = object;
    }
}

bool ServerReceiveCommand(preat_server_t server, uint8_t * command) {
    bool result = (server->rxd->received != 0);
    result &= (server->rxd->data[0] == server->rxd->received);

    if (result) {
        memcpy(command, server->rxd->data, server->rxd->received);
        server->rxd->received = 0;
    }
    return result;
}

bool ServerTransmitResponse(preat_server_t server, uint8_t * response) {
    bool result = (server->txd->data[0] == 0);
    transmission_buffer_t buffer = server->txd;

    if (result) {
        memcpy(buffer->data, response, response[0]);
        buffer->transmited += SciSendData(server->sci, buffer->data, buffer->data[0]);
    }
    return result;
}

/* === End of documentation ==================================================================== */

/** @} End of module definition for doxygen */
