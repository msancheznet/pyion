/* ============================================================================
 * Experimental Python C Extension to interface with ION's LTP implementation
 * 
 * Author: Marc Sanchez Net
 * Date:   05/30/2019
 * Copyright (c) 2019, California Institute of Technology ("Caltech").  
 * U.S. Government sponsorship acknowledged.
 * =========================================================================== */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ion.h>
#include <zco.h>
#include <ltp.h>
#include <Python.h>

#include "_utils.c"
#include "base_ltp.h"
#include "macros.h"

/* ============================================================================
 * === _ltp module definitions
 * ============================================================================ */

// Docstring for this module
static char module_docstring[] =
    "Extension module to interface Python and Licklider Transmission Protocol in ION.";

// Docstring for the functions being wrapped
static char ltp_attach_docstring[] =
    "Attach to the local LTP engine.";
static char ltp_detach_docstring[] =
    "Detach from the local LTP engine.";
static char ltp_open_docstring[] =
    "Open a connection to the local LTP engine.";
static char ltp_close_docstring[] =
    "Close a connection to the local LTP engine.\n";
static char ltp_send_docstring[] =
    "Send a blob of bytes using LTP.";
static char ltp_receive_docstring[] =
    "Receive a blob of bytes using LTP.";
static char ltp_interrupt_docstring[] =
    "Interrupt the reception of LTP data.";

// Declare the functions to wrap
static PyObject *pyion_ltp_attach(PyObject *self, PyObject *args);
static PyObject *pyion_ltp_detach(PyObject *self, PyObject *args);
static PyObject *pyion_ltp_open(PyObject *self, PyObject *args);
static PyObject *pyion_ltp_close(PyObject *self, PyObject *args);
static PyObject *pyion_ltp_send(PyObject *self, PyObject *args);
static PyObject *pyion_ltp_receive(PyObject *self, PyObject *args);
static PyObject *pyion_ltp_interrupt(PyObject *self, PyObject *args);

// Define member functions of this module
static PyMethodDef module_methods[] = {
    {"ltp_attach", pyion_ltp_attach, METH_VARARGS, ltp_attach_docstring},
    {"ltp_detach", pyion_ltp_detach, METH_VARARGS, ltp_detach_docstring},
    {"ltp_open", pyion_ltp_open, METH_VARARGS, ltp_open_docstring},
    {"ltp_close", pyion_ltp_close, METH_VARARGS, ltp_close_docstring},
    {"ltp_send", pyion_ltp_send, METH_VARARGS, ltp_send_docstring},
    {"ltp_receive", pyion_ltp_receive, METH_VARARGS, ltp_receive_docstring},
    {"ltp_interrupt", pyion_ltp_interrupt, METH_VARARGS, ltp_interrupt_docstring},
    {NULL, NULL, 0, NULL}
};

/* ============================================================================
 * === Define _bp as a Python module
 * ============================================================================ */

PyMODINIT_FUNC PyInit__ltp(void) {
    // Define variables
    PyObject *module;

    // Define module configuration parameters
    static struct PyModuleDef moduledef = {
        PyModuleDef_HEAD_INIT,
        "_ltp",
        module_docstring,
        -1,
        module_methods,
        NULL,
        NULL,
        NULL,
        NULL};

    // Create the module
    module = PyModule_Create(&moduledef);

    // If module creation failed, return error
    if (!module) return NULL;

    return module;
}

/* ============================================================================
 * === Attach/Detach Functions
 * ============================================================================ */

static PyObject *pyion_ltp_attach(PyObject *self, PyObject *args) {
    // Initialize variables
    int ok;

    // Release GIL and attach
    Py_BEGIN_ALLOW_THREADS;                                
    ok = base_ltp_attach();
    Py_END_ALLOW_THREADS   
    
    // Error handling
    if (ok < 0) {
        pyion_SetExc(PyExc_SystemError, "Cannot attach to LTP engine. Is ION running on this host?");
        return NULL;
    }

    // Return True to indicate success
    Py_RETURN_TRUE;
}

static PyObject *pyion_ltp_detach(PyObject *self, PyObject *args) {
    // Initialize variables
    int ok;

    // Release GIL and detach
    Py_BEGIN_ALLOW_THREADS                                
    ok = base_ltp_detach();
    Py_END_ALLOW_THREADS

    // Error handling
    if (ok < 0) {
        pyion_SetExc(PyExc_SystemError, "Cannot detach from LTP engine.");
        return NULL;
    }

    // Return True to indicate success
    Py_RETURN_TRUE;
}

/* ============================================================================
 * === Open/Close Endpoint Functions
 * ============================================================================ */

static PyObject *pyion_ltp_open(PyObject *self, PyObject *args) {
    // Define variables
    unsigned int clientId;
    int ok;
    LtpSAP *state;

    // Parse the input tuple. Raises error automatically if not possible
    if (!PyArg_ParseTuple(args, "I", &clientId))
        return NULL;

    // Release GIL and open 
    Py_BEGIN_ALLOW_THREADS                                
    ok = base_ltp_open(&state, clientId);
    Py_END_ALLOW_THREADS

    // Handle errors
    switch(ok) {
        case -1:
            pyion_SetExc(PyExc_RuntimeError, "Cannot malloc for LTP state.");
            break;
        case -2:
            pyion_SetExc(PyExc_ConnectionError, "Cannot open LTP client access point.");
            break;
    }

    // If error, return
    if (ok < PYION_OK) return NULL;

    // Return the memory address of the SAP for this endpoint as an unsined long
    PyObject *ret = Py_BuildValue("k", state);

    return ret;
}

static PyObject *pyion_ltp_close(PyObject *self, PyObject *args) {
    // Define variables
    LtpSAP *state;
    int ok;

    // Parse the input tuple. Raises error automatically if not possible
    if (!PyArg_ParseTuple(args, "k", (unsigned long *)&state))
        return NULL;

    // Release GIL and close
    Py_BEGIN_ALLOW_THREADS                                
    ok = base_ltp_close(state);
    Py_END_ALLOW_THREADS

    // Handle errors
    if (ok < PYION_OK) {
        pyion_SetExc(PyExc_RuntimeError, "Cannot close LTP access point.");
        return NULL;
    }
    
    Py_RETURN_NONE;
}

/* ============================================================================
 * === Interrupt Endpoint Functions
 * ============================================================================ */

static PyObject *pyion_ltp_interrupt(PyObject *self, PyObject *args) {
    // Define variables
    LtpSAP *state;

    // Parse the input tuple. Raises error automatically if not possible
    if (!PyArg_ParseTuple(args, "k", (unsigned long *)&state))
        return NULL;

    // Release GIL and interrupt
    Py_BEGIN_ALLOW_THREADS                                
    int ok = base_ltp_interrupt(state);
    Py_END_ALLOW_THREADS

    Py_RETURN_NONE;
}

/* ============================================================================
 * === Send Functionality
 * ============================================================================ */


static PyObject *pyion_ltp_send(PyObject *self, PyObject *args) {
    // Define variables
    LtpSAP              *state;
    unsigned long long  destEngineId;
    LtpSessionId        sessionId;
    char                *data;
    int                 data_size, ok;

    // Parse input arguments. First one is SAP memory address for this endpoint
    if (!PyArg_ParseTuple(args, "kKs#", (unsigned long *)&state, &destEngineId, &data, &data_size))
        return NULL;

    // Release GIL and send data
    Py_BEGIN_ALLOW_THREADS                                
    ok = base_ltp_send(state, destEngineId, data, data_size);
    Py_END_ALLOW_THREADS

    // Handle errors
    switch(ok) {
        case PYION_SDR_ERR:
            pyion_SetExc(PyExc_RuntimeError, "SDR memory could not be allocated.");
            break;
        case PYION_ZCO_ERR:
            pyion_SetExc(PyExc_ConnectionError, "Cannot create ZCO object.");
            break;
        case PYION_IO_ERR:
            pyion_SetExc(PyExc_ConnectionError, "Error while sending the data through LTP.");
            break;
    }

    // If error, return
    if (ok < PYION_OK) return NULL;

    // If you reach this point, everything is ok
    Py_RETURN_NONE;
}

/* ============================================================================
 * === Receive Functionality
 * ============================================================================ */

static PyObject *pyion_ltp_receive(PyObject *self, PyObject *args) {
    // Define variables
    LtpSAP   *state;
    RxPayload msg = {NULL, 0, 0};
    msg.payload = msg.payload_prealloc;
    int ok;

    // Parse the input tuple. Raises error automatically if not possible
    if (!PyArg_ParseTuple(args, "k", (unsigned long *)&state))
        return NULL;

    // Mark as running
    state->status = SAP_RUNNING;

    // Release GIL and receive data
    Py_BEGIN_ALLOW_THREADS                                
    ok = base_ltp_receive(state, &msg);
    Py_END_ALLOW_THREADS

    // Handle errors
    switch(ok) {
        case -1:
            pyion_SetExc(PyExc_RuntimeError, "LTP import session cancelled.");
            break;
        case -2:
            pyion_SetExc(PyExc_RuntimeError, "LTP export session cancelled.");
            break;
        case -3:
            pyion_SetExc(PyExc_NotImplementedError, "An LTP block cannot have green parts.");
            break;
        case -4:
            pyion_SetExc(PyExc_NotImplementedError, "Only red part of LTP is supported.");
            break;
        case -5:
            pyion_SetExc(PyExc_NotImplementedError, "Error extracting data from block.");
            break;
        case PYION_CONN_ABORTED_ERR:
            pyion_SetExc(PyExc_ConnectionAbortedError, "LTP reception closed.");
            break;
        case PYION_IO_ERR:
            pyion_SetExc(PyExc_ConnectionError, "Error getting LTP notice or segment.");
            break;
        case PYION_SDR_ERR:
            pyion_SetExc(PyExc_MemoryError, "SDR memory could not be read.");
            break;
    }

    // If error, return
    if (ok < PYION_OK) return NULL;

    // Build return object
    PyObject* ret = Py_BuildValue("y#", msg.payload, msg.len);

    // If you allocated memory for this payload, free it here
    if (msg.do_malloc) free(msg.payload);

    // Return value
    return ret;
}

/* ============================================================================
 * === EOF
 * ============================================================================ */