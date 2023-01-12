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

/* === Macros definitions ====================================================================== */

/* === Private data type declarations ========================================================== */

/* === Private variable declarations =========================================================== */
struct {
    bool called;
    uint8_t param_value;
    preat_error_t result;
} single_param_register;

/* === Private function declarations =========================================================== */

preat_error_t SingleParam(const preat_parameter_t parameters, uint8_t count);

/* === Public variable definitions ============================================================= */

/* === Private variable definitions ============================================================ */

// clang-format off
static const uint8_t ACK_NO_ERROR[]          = {0x05, 0x00, 0x00, 0xa1, 0xb5};
static const uint8_t NACK_CRC_ERROR[]        = {0x07, 0x00, 0x11, 0x10, 0x01, 0xcc, 0x08};
static const uint8_t NACK_METHOD_ERROR[]     = {0x07, 0x00, 0x11, 0x10, 0x02, 0x6e, 0xe2};
static const uint8_t NACK_PARAMETERS_ERROR[] = {0x07, 0x00, 0x11, 0x10, 0x03, 0xbf, 0x97};
// clang-format on

/* === Private function implementation ========================================================= */

preat_error_t SingleParam(const preat_parameter_t parameters, uint8_t count) {
    single_param_register.called = true;
    single_param_register.param_value = (uint8_t)parameters[0].value;
    return single_param_register.result;
}

/* === Public function implementation ========================================================= */

void suiteSetUp(void) {
    PreatRegisterOutput(0x10, SingleParam, SINGLE_UINT8_PARAM);
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

    single_param_register.called = false;
    single_param_register.param_value = 0xFF;
    single_param_register.result = PREAT_NO_ERROR;

    PreatExecute(frame);
    TEST_ASSERT_TRUE(single_param_register.called);
    TEST_ASSERT_EQUAL(0x01, single_param_register.param_value);
    TEST_ASSERT_EQUAL_MEMORY(ACK_NO_ERROR, frame, sizeof(ACK_NO_ERROR));
}

/* === End of documentation ==================================================================== */

/** @} End of module definition for doxygen */
