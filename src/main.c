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
 ** @brief Application implementation
 **
 ** @addtogroup ruwaq ruwaq
 ** @brief Firmware for Remote Board to Excecution of Automated Tests
 ** @{ */

/* === Headers files inclusions =============================================================== */

#include "FreeRTOS.h"
#include "task.h"
#include "event_groups.h"

#include "board.h"
#include "gpio.h"
#include "preat.h"
#include "serial.h"
#include <stdint.h>
#include <stddef.h>

/* === Macros definitions ====================================================================== */

/* === Private data type declarations ========================================================== */

/* === Private variable declarations =========================================================== */

EventGroupHandle_t inputs_events;

/* === Private function declarations =========================================================== */

/* === Public variable definitions ============================================================= */

/* === Private variable definitions ============================================================ */

/* === Private function implementation ========================================================= */

event_flags_t AssertWaitEvents(event_flags_t events, uint32_t timeout, bool wait_for_all) {
    EventBits_t result;

    xEventGroupClearBits(inputs_events, events);
    result = xEventGroupWaitBits(inputs_events, events, pdTRUE, wait_for_all ? pdTRUE : pdFALSE,
                                 pdMS_TO_TICKS(timeout));
    return result;
}

void AssertSetEvent(event_id_t id) {
    BaseType_t result, scheduling;

    scheduling = pdFALSE;
    result = xEventGroupSetBitsFromISR(inputs_events, id, &scheduling);
    if (result != pdFAIL) {
        portYIELD_FROM_ISR(scheduling);
    }
}

static void ServerEvent(preat_server_t server, void * object) {
    TaskHandle_t task = object;
    BaseType_t result, scheduling;

    scheduling = pdFALSE;
    result = xTaskResumeFromISR(task);
    if (result != pdFAIL) {
        portYIELD_FROM_ISR(scheduling);
    }
}

void ServerTask(void * object) {
    static uint8_t frame[64] = {0};
    preat_server_t server = object;

    RegisterGpioMethods();

    while (true) {
        vTaskSuspend(NULL);
        if (ServerReceiveCommand(server, frame)) {
            PreatExecute(frame);
            ServerTransmitResponse(server, frame);
        }
    }
}

/* === Public function implementation ========================================================== */

int main(void) {
    struct hal_sci_pins_s server_pins = {0};
    preat_server_t server;
    TaskHandle_t task;

    BoardSetup();

    server_pins.txd_pin = HAL_PIN_P7_1;
    server_pins.rxd_pin = HAL_PIN_P7_2;
    server = ServerStartSerial(HAL_SCI_USART2, &server_pins);

    inputs_events = xEventGroupCreate();
    xTaskCreate(ServerTask, "PreatServer", 2048, (void *)server, tskIDLE_PRIORITY + 1, &task);
    ServerSetEventHandler(server, ServerEvent, task);

    /* Arranque del sistema operativo */
    vTaskStartScheduler();

    /* vTaskStartScheduler solo retorna si se detiene el sistema operativo */
    while (true) {
    }

    /* El valor de retorno es solo para evitar errores en el compilador*/
    return 0;
}

/* === End of documentation ==================================================================== */

/** @} End of module definition for doxygen */
