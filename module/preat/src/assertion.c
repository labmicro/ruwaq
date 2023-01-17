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
 ** @brief Assertion for inputs events implementation
 **
 ** @addtogroup preat PREAT
 ** @brief Protocol for Remote Excecution of Automated Tests
 ** @{ */

/* === Headers files inclusions =============================================================== */

#include "assertion.h"

/* === Macros definitions ====================================================================== */

/* === Private data type declarations ========================================================== */

/**
 * @brief Structure with input handler information
 */
typedef struct input_handler_s {
    input_cleanup_t cleanup; /**< Function to stop the input send events to the assertion */
    input_state_t state;     /**< Pointer to private structure with input handler state */
} input_handler_t;

/**
 * @brief Structure with asertion information
 */
typedef struct assertion_s {
    input_handler_t handlers[8]; /**< Array with input handlerd to stop send events */
    uint32_t delay;              /**< Initial delay that must elapse without receiving events */
    uint32_t timeout;            /**< Maximum waiting time to receive events */
    uint8_t declared_inputs;     /**< Number of inputs declared at the start of the assertion */
    uint8_t defined_inputs;      /**< Number of inputs currently registered on the assertion */
    bool active;                 /**< Flag to indicate that an assert is started */
} * assertion_t;

/* === Private variable declarations =========================================================== */

/* === Private function declarations =========================================================== */

/* === Public variable definitions ============================================================= */

/* === Private variable definitions ============================================================ */

/**
 * @brief Variable with asertion information
 */
static struct assertion_s assertion[1] = {0};

/* === Private function implementation ========================================================= */

void AssertClean(void) {
    assertion->active = false;
}

preat_error_t AssertStart(const preat_parameter_t parameters, uint8_t count) {
    preat_error_t result = PREAT_REDEFINED_ERROR;

    if (!assertion->active) {
        assertion->delay = parameters[0].value;
        assertion->timeout = parameters[1].value;
        assertion->declared_inputs = (uint8_t)parameters[2].value;
        assertion->defined_inputs = 0;
        assertion->active = true;
        result = PREAT_NO_ERROR;
    }

    return result;
}

event_id_t AssertRegisterEvent(input_cleanup_t cleanup, input_state_t state) {
    event_id_t result = ASSERT_EVENT_INVALID_ID;
    if (assertion->defined_inputs < assertion->declared_inputs) {
        input_handler_t * handler = &(assertion->handlers[assertion->defined_inputs]);
        result = (1 << assertion->defined_inputs);
        handler->cleanup = cleanup;
        handler->state = state;
        assertion->defined_inputs++;
    }
    return result;
}

preat_error_t AssertExecute(preat_method_t handler, preat_parameter_t parameters, uint8_t count) {
    preat_error_t result = PREAT_UNDEFINED_ERROR;
    uint8_t index;
    event_flags_t expected;

    if (assertion->defined_inputs == assertion->declared_inputs) {
        result = handler(parameters, count);
    }
    if (result == PREAT_NO_ERROR) {
        expected = (1 << assertion->defined_inputs) - 1;
        if (AssertWaitEvents(expected, assertion->delay, false) != 0) {
            result = PREAT_TOO_EARLY_ERROR;
        }
    }
    if (result == PREAT_NO_ERROR) {
        if (AssertWaitEvents(expected, assertion->timeout, true) != expected) {
            result = PREAT_TIMEOUT_ERROR;
        }
    }
    for (index = 0; index < assertion->defined_inputs; index++) {
        input_handler_t * handler = &(assertion->handlers[index]);
        handler->cleanup(handler->state);
    }

    AssertClean();

    return result;
}

bool AssertIsDefined(void) {
    return assertion->active;
}

/* === Public function implementation ========================================================== */

/* === End of documentation ==================================================================== */

/** @} End of module definition for doxygen */
