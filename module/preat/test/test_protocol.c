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

/** \brief Remote execution protocol unit tests
 **
 ** \addtogroup preat PREAT
 ** \brief Protocol for Remote Excecution of Automated Tests
 ** @{ */

/* === Headers files inclusions =============================================================== */

#include "unity.h"
#include "crc.h"
#include "protocol.h"
#include "assertion.h"
#include <string.h>

/* === Macros definitions ====================================================================== */

#define FakeReset(var) memset(&var, 0, sizeof(var));

/* === Private data type declarations ========================================================== */

/* === Private variable declarations =========================================================== */

struct input_state_s {
    uint32_t dummy_field;
} fake_state;

static struct fake_cleanup_s {
    bool called;
    input_state_t state;
} fake_cleanup;

static struct fake_input_s {
    bool called;
    uint8_t parameter;
    uint8_t count;
    event_id_t event_id;
    preat_error_t result;
} fake_input;

static struct fake_output_s {
    bool called;
    uint8_t parameter;
    uint8_t count;
    preat_error_t result;
} fake_output;

static struct fake_events_s {
    uint8_t called;
    struct events_call_s {
        event_flags_t events;
        uint32_t timeout;
        bool wait_for_all;
        event_flags_t result;
    } calls[8];
} fake_events;

/* === Private function declarations =========================================================== */

preat_error_t FakeInput(const preat_parameter_t parameters, uint8_t count);

preat_error_t FakeOutput(const preat_parameter_t parameters, uint8_t count);

/* === Public variable definitions ============================================================= */

/* === Private variable definitions ============================================================ */

// clang-format off
static const uint8_t ACK_NO_ERROR[]          = {0x05, 0x00, 0x00, 0xa1, 0xb5};
static const uint8_t NACK_CRC_ERROR[]        = {0x07, 0x00, 0x11, 0x10, 0x01, 0xcc, 0x08};
static const uint8_t NACK_METHOD_ERROR[]     = {0x07, 0x00, 0x11, 0x10, 0x02, 0x6e, 0xe2};
static const uint8_t NACK_PARAMETERS_ERROR[] = {0x07, 0x00, 0x11, 0x10, 0x03, 0xbf, 0x97};
// clang-format on

/* === Private function implementation ========================================================= */

void FakeCleanup(input_state_t state) {
    fake_cleanup.called = true;
    fake_cleanup.state = state;
}

preat_error_t FakeInput(const preat_parameter_t parameters, uint8_t count) {
    fake_input.called = true;
    fake_input.parameter = (uint8_t)parameters[0].value;
    fake_input.count = count;
    fake_input.event_id = AssertRegisterEvent(FakeCleanup, &fake_state);
    return fake_input.result;
}

preat_error_t FakeOutput(const preat_parameter_t parameters, uint8_t count) {
    fake_output.called = true;
    fake_output.parameter = (uint8_t)parameters[0].value;
    fake_output.count = count;
    return fake_output.result;
}

/* === Public function implementation ========================================================= */

event_flags_t AssertWaitEvents(event_flags_t events, uint32_t timeout, bool wait_for_all) {
    event_flags_t result = 0;

    if (fake_events.called < sizeof(fake_events.calls) / sizeof(fake_events.calls[0]) - 1) {
        fake_events.calls[fake_events.called].events = events;
        fake_events.calls[fake_events.called].timeout = timeout;
        fake_events.calls[fake_events.called].wait_for_all = wait_for_all;
        result = fake_events.calls[fake_events.called].result;
        fake_events.called++;
    } else {
        TEST_FAIL_MESSAGE("No more space to save calls");
    }
    return result;
}

void suiteSetUp(void) {
    PreatRegister(0x10, true, FakeOutput, SINGLE_UINT8_PARAM);
    PreatRegister(0x15, false, FakeInput, SINGLE_UINT8_PARAM);
}

void setUp(void) {
    FakeReset(fake_input);
    FakeReset(fake_output);
    FakeReset(fake_events);
    FakeReset(fake_cleanup);
}

void test_frame_has_crc_error(void) {
    uint8_t frame[64] = {0x07, 0x01, 0x01, 0x10, 0x01, 0xb5, 0x00};

    PreatExecute(frame);
    TEST_ASSERT_EQUAL_MEMORY(NACK_CRC_ERROR, frame, sizeof(NACK_CRC_ERROR));
}

void test_execute_undefined_function(void) {
    uint8_t frame[64] = {0x07, 0x02, 0x01, 0x10, 0x01, 0xa2, 0xcf};

    PreatExecute(frame);
    TEST_ASSERT_EQUAL_MEMORY(NACK_METHOD_ERROR, frame, sizeof(NACK_METHOD_ERROR));
}

void test_execute_funcion_with_less_parameters(void) {
    uint8_t frame[] = {0x05, 0x01, 0x00, 0xe2, 0x7f};

    PreatExecute(frame);
    TEST_ASSERT_EQUAL_MEMORY(NACK_PARAMETERS_ERROR, frame, sizeof(NACK_PARAMETERS_ERROR));
}

void test_execute_funcion_with_more_parameters(void) {
    uint8_t frame[] = {0x08, 0x01, 0x02, 0x11, 0x01, 0x02, 0x3b, 0x88};

    PreatExecute(frame);
    TEST_ASSERT_EQUAL_MEMORY(NACK_PARAMETERS_ERROR, frame, sizeof(NACK_PARAMETERS_ERROR));
}

void test_execute_single_parameter_funcion(void) {
    uint8_t frame[] = {0x07, 0x01, 0x01, 0x10, 0x01, 0xb5, 0xa3};

    fake_output.called = false;
    fake_output.parameter = 0xFF;
    fake_output.result = PREAT_NO_ERROR;

    PreatExecute(frame);
    TEST_ASSERT_TRUE(fake_output.called);
    TEST_ASSERT_EQUAL(0x01, fake_output.parameter);
    TEST_ASSERT_EQUAL_MEMORY(ACK_NO_ERROR, frame, sizeof(ACK_NO_ERROR));
}

void test_execute_assert_single_condition(void) {
    uint8_t frames[3][17] = {
        {0x11, 0x00, 0x54, 0x33, 0x00, 0x00, 0x00, 0x64, 0x00, 0x00, 0x13, 0x88, 0x11, 0x01, 0x00,
         0xcd, 0x2c},
        {0x07, 0x01, 0x51, 0x10, 0x03, 0xB1, 0xFA},
        {0x07, 0x01, 0x01, 0x10, 0x01, 0xb5, 0xa3},
    };

    PreatExecute(frames[0]);
    TEST_ASSERT_EQUAL_MEMORY(ACK_NO_ERROR, frames[0], sizeof(ACK_NO_ERROR));

    PreatExecute(frames[1]);
    TEST_ASSERT_EQUAL_MEMORY(ACK_NO_ERROR, frames[0], sizeof(ACK_NO_ERROR));

    fake_events.calls[1].result = fake_input.event_id;
    PreatExecute(frames[2]);
    TEST_ASSERT_EQUAL_MEMORY(ACK_NO_ERROR, frames[0], sizeof(ACK_NO_ERROR));

    TEST_ASSERT_TRUE(fake_output.called);
    TEST_ASSERT_EQUAL(0x01, fake_output.parameter);
    TEST_ASSERT_EQUAL(1, fake_output.count);

    TEST_ASSERT_TRUE(fake_input.called);
    TEST_ASSERT_EQUAL(3, fake_input.parameter);
    TEST_ASSERT_EQUAL(1, fake_input.count);

    TEST_ASSERT_EQUAL(2, fake_events.called);
    TEST_ASSERT_EQUAL(100, fake_events.calls[0].timeout);
    TEST_ASSERT_FALSE(fake_events.calls[0].wait_for_all);
    TEST_ASSERT_EQUAL(5000, fake_events.calls[1].timeout);
    TEST_ASSERT_TRUE(fake_events.calls[1].wait_for_all);

    TEST_ASSERT_TRUE(fake_cleanup.called);
    TEST_ASSERT_EQUAL(&fake_state, fake_cleanup.state);
}

/* === End of documentation ==================================================================== */

/** @} End of module definition for doxygen */
