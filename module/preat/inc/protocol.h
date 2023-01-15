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

#ifndef PROTOCOL_H
#define PROTOCOL_H

/** @file
 ** @brief Remote execution protocol declarations
 **
 ** @addtogroup preat PREAT
 ** @brief Protocol for Remote Excecution of Automated Tests
 ** @{ */

/* === Headers files inclusions ================================================================ */

#include <stdbool.h>
#include <stdint.h>

/* === Cabecera C++ ============================================================================ */

#ifdef __cplusplus
extern "C" {
#endif

/* === Public macros definitions =============================================================== */

/**
 * @brief Result of the execution of a method
 */
typedef enum preat_error_e {
    PREAT_NO_ERROR = 0x00,
    PREAT_CRC_ERROR = 0x01,
    PREAT_METHOD_ERROR = 0x02,
    PREAT_PARAMETERS_ERROR = 0x03,
    PREAT_TOO_EARLY_ERROR = 0x04,
    PREAT_TIMEOUT_ERROR = 0x05,
    PREAT_UNDEFINED_ERROR = 0x06,
    PREAT_REDEFINED_ERROR = 0x07,
    PREAT_GENERIC_ERROR = 0xFF,
} preat_error_t;

/**
 * @brief Data type of a method parameter
 */
typedef enum preat_type_e {
    TYPE_UNDEFINED = 0x00,
    TYPE_UINT8 = 0x01,
    TYPE_UINT16 = 0x02,
    TYPE_UINT32 = 0x03,
    TYPE_BLOB = 0x07,
    TYPE_BINARY = 0x80,
} preat_type_t;

/**
 * @brief Parameter definition to execute a method
 */
typedef struct preat_parameter_s {
    preat_type_t type;
    uint32_t value;
} * preat_parameter_t;

/**
 * @brief Callback function to implement an method to set a condition by an output action
 *
 * @param  parameters       Pointer to list of parameters to excecute the method
 * @param  count            Count of parameters sended in the list of parameters
 * @return preat_error_t    Result of the execution of the method
 */
typedef preat_error_t (*preat_method_t)(const preat_parameter_t parameters, uint8_t count);

/* === Public data type declarations =========================================================== */

/* === Public variable declarations ============================================================ */

/**
 * @brief Constant with a single parameter method declaration
 */
extern const preat_type_t SINGLE_UINT8_PARAM[];

/* === Public function declarations ============================================================ */

/**
 * @brief Register a function to implemente an protocol method to drive outputs actions
 *
 * @param   id          Unique number to idetify the method in the protocol
 * @param   output      Function implement an method for control outputs
 * @param   handler     Function to call when protocol method is excecuted
 * @param   parameters  Definition of the parameters required by the function
 * @return  true        Function could be successfully registered
 * @return  false       Function failed to register successfully
 */
bool PreatRegister(uint16_t id, bool output, preat_method_t handler,
                   preat_type_t const * parameters);

/**
 * @brief Decode a protocol frame and executes the corresponding method
 *
 * @param   frame       Received frame with a command to execute a method
 */
void PreatExecute(uint8_t * frame);

/* === End of documentation ==================================================================== */

#ifdef __cplusplus
}
#endif

/** @} End of module definition for doxygen */

#endif /* PROTOCOL_H */
