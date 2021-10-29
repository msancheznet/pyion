#ifndef MACROS_H_
#define MACROS_H_

#include <stdlib.h>
#include <string.h>
#include <Python.h>

#include "ion.h"

#include "return_codes.h"

/* ============================================================================
 * === Define global variables
 * ============================================================================ */

#define ERR_MSG_LENGTH 150

/* ============================================================================
 * === Define macros
 * ============================================================================ */

#define OK_RETURN \
    return PYION_OK;

#define ERR_VARS \
    char err_msg[ERR_MSG_LENGTH]; \
    err_msg[0] = 0; \
    PyObject* exc;

#define ERR_RETURN \
    PyErr_SetString(exc, err_msg); \
    return NULL;

#define ERR_DEFAULT \
    case PYION_SDR_ERR: \
        exc = PyExc_MemoryError; \
        break; \
    default: \
        exc = PyExc_RuntimeError; \
        strcpy(err_msg, "Unknown error");

#define SDR_GET \
    Sdr sdr = getIonsdr(); \
    if (sdr == NULL) { \
        strcpy(err_msg, "Cannot locate SDR."); \
        return PYION_SDR_ERR; \
    }

#define SDR_BEGIN_XN \
    if (!sdr_begin_xn(sdr)) { \
        strcpy(err_msg, "Cannot start SDR transaction."); \
        return PYION_SDR_ERR; \
    }

#define SDR_END_XN \
    if (sdr_end_xn(sdr) < 0) { \
        strcpy(err_msg, "Cannot end SDR transaction."); \
        return PYION_SDR_ERR; \
    }

#define SDR_EXIT_XN \
    sdr_exit_xn(sdr);

#endif