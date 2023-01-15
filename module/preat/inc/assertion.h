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

#ifndef ASSERTION_H
#define ASSERTION_H

/** @file
 ** @brief Assertion for inputs events declarations
 **
 ** @addtogroup preat PREAT
 ** @brief Protocol for Remote Excecution of Automated Tests
 ** @{ */

/* === Headers files inclusions ================================================================ */

#include "preat.h"
#include <stdbool.h>
#include <stdint.h>

/* === Cabecera C++ ============================================================================ */

#ifdef __cplusplus
extern "C" {
#endif

/* === Public macros definitions =============================================================== */

/**
 * @brief Constant with an invalid event id
 */
#define ASSERT_EVENT_INVALID_ID 0

/* === Public data type declarations =========================================================== */

/**
 * @brief Pointer to private structure with input handler state
 */
typedef struct input_state_s * input_state_t;

/**
 * @brief Function to stop an input from sending more events to the assertion
 *
 * @param  state    Pointer to private structure with input handler state
 */
typedef void (*input_cleanup_t)(input_state_t state);

/**
 * @brief Data type to store the event id associated with an input
 */
typedef uint32_t event_id_t;

/**
 * @brief Data type to indicate the events that occurred during a wait
 */
typedef event_id_t event_flags_t;

/* === Public variable declarations ============================================================ */

/* === Public function declarations ============================================================ */

/**
 * @brief Function to clear an uncompleted assert by an error on the process
 */
void AssertClean(void);

/**
 * @brief Function to initiate an assertion about the behavior of inputs
 *
 * @param  parameters       Pointer to array with method parameters
 * @param  count            Count of parameters defined in the array
 * @return preat_error_t    Error code with the result of the new assertion definition
 */
preat_error_t AssertStart(const preat_parameter_t parameters, uint8_t count);

/**
 * @brief Function to register an input method to send an event to an asertion
 *
 * @param  cleanup      Function to call to stop an input from sending more events to the assertion
 * @param  state        Pointer to private structure with input handler state
 * @return event_id_t   Event id that should use to report that the expected condition has occurred
 */
event_id_t AssertRegisterEvent(input_cleanup_t cleanup, input_state_t state);

/**
 * @brief Function to call an exit method and start the assertion timeout
 *
 * @param  handler          Function that implements the output method that starts the assertion
 * @param  parameters       Pointer to array with output method parameters
 * @param  count            Count of parameters defined in the array
 * @return preat_error_t    Error code with the result of assertion execution
 */
preat_error_t AssertExecute(preat_method_t handler, preat_parameter_t parameters, uint8_t count);

/**
 * @brief Function to inform if an assert was started but not yet executed
 *
 * @return  true    An assert was started but not yet executed
 * @return  false   There is no assertion pending execution
 */
bool AssertIsDefined(void);

/**
 * @brief Function provided by user to set an event from input to assert thread
 *
 * @param  id   Identifier obtained by registering the input as an event
 */
extern void AssertSetEvent(event_id_t id);

/**
 * @brief Function provided by user to pause assert thread while waiting for events
 *
 * @param  events           Flags with the events that wait to be occurred during the assert
 * @param  timeout          Time, in milliseconds, to wait for the arrival of events
 * @param  wait_for_all     Flag to indicate if any or all events are expected
 * @return event_flags_t    Flags with the events that occurred during the wait
 */
extern event_flags_t AssertWaitEvents(event_flags_t events, uint32_t timeout, bool wait_for_all);

/* === End of documentation ==================================================================== */

#ifdef __cplusplus
}
#endif

/** @} End of module definition for doxygen */

#endif /* ASSERTION_H */
