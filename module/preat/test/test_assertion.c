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

/** \brief Assertion for inputs events unit tests
 **
 ** \addtogroup preat PREAT
 ** \brief Protocol for Remote Excecution of Automated Tests
 ** @{ */

/* === Headers files inclusions =============================================================== */

#include "unity.h"
#include "assertion.h"
#include <string.h>

/* === Macros definitions ====================================================================== */

#define DELAY          100

#define TIMEOUT        5000

#define FakeReset(var) memset(&var, 0, sizeof(var));

/* === Private data type declarations ========================================================== */

/* === Private variable declarations =========================================================== */

/* === Private function declarations =========================================================== */

/* === Public variable definitions ============================================================= */

/* === Private variable definitions ============================================================ */

static struct preat_parameter_s fake_parameters[] = {
    {TYPE_UINT8, 3},
};

static struct preat_parameter_s assert_parameters[] = {
    {TYPE_UINT32, DELAY},
    {TYPE_UINT32, TIMEOUT},
    {TYPE_UINT8, 1},
    {TYPE_UINT8, 0},
};

struct input_state_s {
    uint32_t dummy_field;
} fake_state;

static struct fake_cleanup_s {
    bool called;
    input_state_t state;
} fake_cleanup;

static struct fake_method_s {
    bool called;
    preat_parameter_t parameters;
    uint8_t count;
    preat_error_t result;
} fake_method;

static struct fake_events_s {
    uint8_t called;
    struct events_call_s {
        event_flags_t events;
        uint32_t timeout;
        bool wait_for_all;
        event_flags_t result;
    } calls[8];
} fake_events;

/* === Private function implementation ========================================================= */

void FakeCleanup(input_state_t state) {
    fake_cleanup.called = true;
    fake_cleanup.state = state;
}

preat_error_t FakeMethod(const preat_parameter_t parameters, uint8_t count) {
    fake_method.called = true;
    fake_method.parameters = parameters;
    fake_method.count = count;
    return fake_method.result;
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

void setUp(void) {
    FakeReset(fake_method);
    FakeReset(fake_cleanup);
    FakeReset(fake_events);
}

void tearDown(void) {
    AssertClean();
}

void test_assertion_defined_and_event_occurs_as_expected(void) {
    TEST_ASSERT_EQUAL(PREAT_NO_ERROR, AssertStart(assert_parameters, 4));
    event_id_t id = AssertRegisterEvent(FakeCleanup, &fake_state);
    TEST_ASSERT_NOT_EQUAL(ASSERT_EVENT_INVALID_ID, id);

    fake_method.result = PREAT_NO_ERROR;
    fake_events.calls[1].result = id;
    TEST_ASSERT_EQUAL(PREAT_NO_ERROR, AssertExecute(FakeMethod, fake_parameters, 1));

    TEST_ASSERT_TRUE(fake_method.called);
    TEST_ASSERT_EQUAL(fake_parameters, fake_method.parameters);
    TEST_ASSERT_EQUAL(1, fake_method.count);

    TEST_ASSERT_EQUAL(2, fake_events.called);
    TEST_ASSERT_EQUAL(DELAY, fake_events.calls[0].timeout);
    TEST_ASSERT_FALSE(fake_events.calls[0].wait_for_all);
    TEST_ASSERT_EQUAL(TIMEOUT, fake_events.calls[1].timeout);
    TEST_ASSERT_TRUE(fake_events.calls[1].wait_for_all);

    TEST_ASSERT_TRUE(fake_cleanup.called);
    TEST_ASSERT_EQUAL(&fake_state, fake_cleanup.state);
}

void test_assertion_defined_and_event_not_occurs(void) {
    TEST_ASSERT_EQUAL(PREAT_NO_ERROR, AssertStart(assert_parameters, 4));
    event_id_t id = AssertRegisterEvent(FakeCleanup, &fake_state);
    TEST_ASSERT_NOT_EQUAL(ASSERT_EVENT_INVALID_ID, id);

    fake_method.result = PREAT_NO_ERROR;
    TEST_ASSERT_EQUAL(PREAT_TIMEOUT_ERROR, AssertExecute(FakeMethod, fake_parameters, 1));

    TEST_ASSERT_TRUE(fake_method.called);
    TEST_ASSERT_EQUAL(fake_parameters, fake_method.parameters);
    TEST_ASSERT_EQUAL(1, fake_method.count);

    TEST_ASSERT_EQUAL(2, fake_events.called);
    TEST_ASSERT_EQUAL(DELAY, fake_events.calls[0].timeout);
    TEST_ASSERT_FALSE(fake_events.calls[0].wait_for_all);
    TEST_ASSERT_EQUAL(TIMEOUT, fake_events.calls[1].timeout);
    TEST_ASSERT_TRUE(fake_events.calls[1].wait_for_all);

    TEST_ASSERT_TRUE(fake_cleanup.called);
    TEST_ASSERT_EQUAL(&fake_state, fake_cleanup.state);
}

void test_assertion_defined_and_event_occur_before_than_expected(void) {
    TEST_ASSERT_EQUAL(PREAT_NO_ERROR, AssertStart(assert_parameters, 4));
    event_id_t id = AssertRegisterEvent(FakeCleanup, &fake_state);
    TEST_ASSERT_NOT_EQUAL(ASSERT_EVENT_INVALID_ID, id);

    fake_method.result = PREAT_NO_ERROR;
    fake_events.calls[0].result = id;
    TEST_ASSERT_EQUAL(PREAT_TOO_EARLY_ERROR, AssertExecute(FakeMethod, fake_parameters, 1));

    TEST_ASSERT_TRUE(fake_method.called);
    TEST_ASSERT_EQUAL(fake_parameters, fake_method.parameters);
    TEST_ASSERT_EQUAL(1, fake_method.count);

    TEST_ASSERT_EQUAL(1, fake_events.called);

    TEST_ASSERT_TRUE(fake_cleanup.called);
    TEST_ASSERT_EQUAL(&fake_state, fake_cleanup.state);
}
void test_assertion_defined_and_output_method_raises_an_error(void) {
    TEST_ASSERT_EQUAL(PREAT_NO_ERROR, AssertStart(assert_parameters, 4));
    event_id_t id = AssertRegisterEvent(FakeCleanup, &fake_state);
    TEST_ASSERT_NOT_EQUAL(ASSERT_EVENT_INVALID_ID, id);

    fake_method.result = PREAT_PARAMETERS_ERROR;
    TEST_ASSERT_EQUAL(PREAT_PARAMETERS_ERROR, AssertExecute(FakeMethod, fake_parameters, 1));

    TEST_ASSERT_TRUE(fake_method.called);
    TEST_ASSERT_EQUAL(fake_parameters, fake_method.parameters);
    TEST_ASSERT_EQUAL(1, fake_method.count);

    TEST_ASSERT_EQUAL(0, fake_events.called);

    TEST_ASSERT_TRUE(fake_cleanup.called);
    TEST_ASSERT_EQUAL(&fake_state, fake_cleanup.state);
}

void test_start_two_assert_raise_error(void) {
    TEST_ASSERT_EQUAL(PREAT_NO_ERROR, AssertStart(assert_parameters, 4));
    TEST_ASSERT_EQUAL(PREAT_REDEFINED_ERROR, AssertStart(assert_parameters, 4));
}

void test_start_and_execute_without_append_cleanups_raise_error(void) {
    TEST_ASSERT_EQUAL(PREAT_NO_ERROR, AssertStart(assert_parameters, 4));
    TEST_ASSERT_EQUAL(PREAT_UNDEFINED_ERROR, AssertExecute(FakeMethod, fake_parameters, 1));

    TEST_ASSERT_FALSE(fake_method.called);
    TEST_ASSERT_EQUAL(0, fake_events.called);
    TEST_ASSERT_FALSE(fake_cleanup.called);
}

void test_add_more_inputs_to_assert_raise_error(void) {
    TEST_ASSERT_EQUAL(PREAT_NO_ERROR, AssertStart(assert_parameters, 4));

    event_id_t id = AssertRegisterEvent(FakeCleanup, &fake_state);
    TEST_ASSERT_NOT_EQUAL(ASSERT_EVENT_INVALID_ID, id);

    id = AssertRegisterEvent(FakeCleanup, &fake_state);
    TEST_ASSERT_EQUAL(ASSERT_EVENT_INVALID_ID, id);
}

void test_assert_in_undefined_on_start(void) {
    TEST_ASSERT_FALSE(AssertIsDefined());
}

void test_assert_in_pendding_after_start_the_definition(void) {
    TEST_ASSERT_EQUAL(PREAT_NO_ERROR, AssertStart(assert_parameters, 4));
    TEST_ASSERT_TRUE(AssertIsDefined());
}

/* === End of documentation ==================================================================== */

/** @} End of module definition for doxygen */
