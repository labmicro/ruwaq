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
 ** @brief Digital inputs and output functions implementation
 **
 ** @addtogroup ruwaq ruwaq
 ** @brief Firmware for Remote Board to Excecution of Automated Tests
 ** @{ */

/* === Headers files inclusions =============================================================== */

#include "gpio.h"
#include "preat.h"
#include "config.h"
#include "hal.h"
#include <stddef.h>

/* === Macros definitions ====================================================================== */

/* === Private data type declarations ========================================================== */

struct input_state_s {
    hal_gpio_bit_t input;
    event_id_t event_id;
};

/* === Private variable declarations =========================================================== */

/* === Private function declarations =========================================================== */

/* === Public variable definitions ============================================================= */

static hal_gpio_bit_t inputs[GPIO_INPUTS_COUNT];

struct input_state_s input_states[GPIO_INPUTS_COUNT];

static hal_gpio_bit_t outputs[GPIO_OUTPUTS_COUNT];

/* === Private variable definitions ============================================================ */

/* === Private function implementation ========================================================= */

static void GpioInputCleanup(input_state_t state) {
    GpioSetEventHandler(state->input, NULL, NULL, false, false);
}

static void GpioEventsHandler(hal_gpio_bit_t gpio, bool rissing, void * object) {
    input_state_t state = object;

    AssertSetEvent(state->event_id);
}

static preat_error_t ExecuteInput(uint8_t input, bool rissing, bool falling) {
    preat_error_t result = PREAT_NO_ERROR;

    if (input >= GPIO_OUTPUTS_COUNT) {
        result = PREAT_GENERIC_ERROR;
    } else {
        input_state_t state = &input_states[input];
        state->input = inputs[input];
        state->event_id = AssertRegisterEvent(GpioInputCleanup, state);

        if (state->event_id == ASSERT_EVENT_INVALID_ID) {
            result = PREAT_GENERIC_ERROR;
        } else {
            GpioSetEventHandler(state->input, GpioEventsHandler, state, rissing, falling);
        }
    }
    return result;
}

static preat_error_t ExecuteOutput(uint8_t output, void (*action)(hal_gpio_bit_t gpio)) {
    preat_error_t result = PREAT_NO_ERROR;

    if (output >= GPIO_OUTPUTS_COUNT) {
        result = PREAT_GENERIC_ERROR;
    } else {
        action(outputs[output]);
    }
    return result;
}

static preat_error_t HasRissing(const preat_parameter_t parameters, uint8_t count) {
    return ExecuteInput((uint8_t)parameters->value, true, false);
}

static preat_error_t HasFalling(const preat_parameter_t parameters, uint8_t count) {
    return ExecuteInput((uint8_t)parameters->value, false, true);
}

static preat_error_t HasChanged(const preat_parameter_t parameters, uint8_t count) {
    return ExecuteInput((uint8_t)parameters->value, true, true);
}

static preat_error_t ActivateOutput(const preat_parameter_t parameters, uint8_t count) {
    return ExecuteOutput((uint8_t)parameters->value, GpioBitSet);
}

static preat_error_t DeactivateOutput(const preat_parameter_t parameters, uint8_t count) {
    return ExecuteOutput((uint8_t)parameters->value, GpioBitClear);
}

static preat_error_t ToogleOutput(const preat_parameter_t parameters, uint8_t count) {
    return ExecuteOutput((uint8_t)parameters->value, GpioBitToogle);
}

/* === Public function implementation ========================================================== */

bool RegisterGpioMethods(void) {
    uint8_t index;
    bool result = true;

    GpioInputsListInit(inputs, sizeof(inputs) / sizeof(hal_chip_pin_t));
    for (index = 0; index < GPIO_INPUTS_COUNT; index++) {
        GpioSetDirection(inputs[index], false);
    }

    GpioOutputsListInit(outputs, sizeof(outputs) / sizeof(hal_chip_pin_t));
    for (index = 0; index < GPIO_OUTPUTS_COUNT; index++) {
        GpioSetDirection(outputs[index], true);
    }

    result = result && PreatRegister(0x010, true, ActivateOutput, SINGLE_UINT8_PARAM);
    result = result && PreatRegister(0x011, true, DeactivateOutput, SINGLE_UINT8_PARAM);
    result = result && PreatRegister(0x012, true, ToogleOutput, SINGLE_UINT8_PARAM);
    result = result && PreatRegister(0x013, false, HasRissing, SINGLE_UINT8_PARAM);
    result = result && PreatRegister(0x014, false, HasFalling, SINGLE_UINT8_PARAM);
    result = result && PreatRegister(0x015, false, HasChanged, SINGLE_UINT8_PARAM);

    return result;
}

/* === End of documentation ==================================================================== */

/** @} End of module definition for doxygen */
