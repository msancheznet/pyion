/* ============================================================================
 * Interface between the ``base_bp`` and Python
 *
 * Author: Marc Sanchez Net
 * Date:   03/22/2021
 * Copyright (c) 2021, California Institute of Technology ("Caltech").  
 * U.S. Government sponsorship acknowledged.
 * =========================================================================== */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <bp.h>
#include <Python.h>

#include "_utils.c"
#include "base_bp.h"
#include "macros.h"

/* ============================================================================
 * === _bp module definitions
 * ============================================================================ */

// Docstring for this module
static char module_docstring[] =
    "Extension module to interface Python and Bundle Protocol in ION.";

// Docstring for the functions being wrapped
static char bp_attach_docstring[] =
    "Attach to BP agent.";
static char bp_detach_docstring[] =
    "Detach from BP agent.";
static char bp_open_docstring[] =
    "Open an endpoint in this BP agent.\n"
    "Arguments\n"
    "---------\n"
    "String [s]: Endpoint ID"
    "Return\n"
    "------\n"
    "Long [k]: Memory address of SAP opened";
static char bp_close_docstring[] =
    "Close an endpoint in this BP agent.\n"
    "Arguments\n"
    "---------\n"
    "Long [k]: Memory address of SAP to close";
static char bp_send_docstring[] =
    "Send a blob of bytes using bp_send.\n"
    "Arguments\n"
    "---------\n"
    "Long [k]: SAP memory address of endpoint\n"
    "String [s]: Destination EID\n"
    "String or None [z]: Report EID\n"
    "Int [i]: Time-to-live [sec]\n"
    "Int [i]: BP priority\n"
    "Int [i]: BP custody\n"
    "Int [i]: Report flags\n"
    "Int [i]: Acknowledgement required\n"
    "Bytes-like object [s#]: data";
static char bp_receive_docstring[] =
    "Receive a blob of bytes using bp_send.\n"
    "Arguments\n"
    "---------\n"
    "Long [k]: Memory address of SAP to receive from";
static char bp_interrupt_docstring[] =
    "Interrupt an endpoint that is blocked while receiving.\n"
    "Arguments\n"
    "---------\n"
    "Long [k]: Memory address of SAP to interrupt";

// Declare the functions to wrap
static PyObject *pyion_bp_attach(PyObject *self, PyObject *args);
static PyObject *pyion_bp_detach(PyObject *self, PyObject *args);
static PyObject *pyion_bp_open(PyObject *self, PyObject *args);
static PyObject *pyion_bp_close(PyObject *self, PyObject *args);
static PyObject *pyion_bp_send(PyObject *self, PyObject *args);
static PyObject *pyion_bp_receive(PyObject *self, PyObject *args);
static PyObject *pyion_bp_interrupt(PyObject *self, PyObject *args);

// Define member functions of this module
static PyMethodDef module_methods[] = {
    {"bp_attach", pyion_bp_attach, METH_VARARGS, bp_attach_docstring},
    {"bp_detach", pyion_bp_detach, METH_VARARGS, bp_detach_docstring},
    {"bp_open", pyion_bp_open, METH_VARARGS, bp_open_docstring},
    {"bp_close", pyion_bp_close, METH_VARARGS, bp_close_docstring},
    {"bp_send", pyion_bp_send, METH_VARARGS, bp_send_docstring},
    {"bp_receive", pyion_bp_receive, METH_VARARGS, bp_receive_docstring},
    {"bp_interrupt", pyion_bp_interrupt, METH_VARARGS, bp_interrupt_docstring},
    {NULL, NULL, 0, NULL}
};

/* ============================================================================
 * === Define _bp as a Python module
 * ============================================================================ */

PyMODINIT_FUNC PyInit__bp(void) {
    // Define variables
    PyObject *module;

    // Define module configuration parameters
    static struct PyModuleDef moduledef = {
        PyModuleDef_HEAD_INIT,
        "_bp",
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

    // Add constants to be used in Python interface
    PyModule_AddIntMacro(module, BP_BULK_PRIORITY);
    PyModule_AddIntMacro(module, BP_STD_PRIORITY);
    PyModule_AddIntMacro(module, BP_EXPEDITED_PRIORITY);
    PyModule_AddIntMacro(module, BP_RECEIVED_RPT);
    PyModule_AddIntMacro(module, BP_CUSTODY_RPT);
    PyModule_AddIntMacro(module, BP_FORWARDED_RPT);
    PyModule_AddIntMacro(module, BP_DELIVERED_RPT);
    PyModule_AddIntMacro(module, BP_DELETED_RPT);
    PyModule_AddIntMacro(module, BP_MINIMUM_LATENCY);
    PyModule_AddIntMacro(module, BP_BEST_EFFORT);
    PyModule_AddIntMacro(module, BP_RELIABLE);
    PyModule_AddIntMacro(module, BP_RELIABLE_STREAMING);
    PyModule_AddIntConstant(module, "NoCustodyRequested", NoCustodyRequested);
    PyModule_AddIntConstant(module, "SourceCustodyOptional", SourceCustodyOptional);
    PyModule_AddIntConstant(module, "SourceCustodyRequired", SourceCustodyRequired);

    return module;
}

/* ============================================================================
 * === Attach/Detach Functions
 * =================================s=========================================== */

static PyObject *pyion_bp_attach(PyObject *self, PyObject *args) {
    // Initialize variables
    int ok;

    // Release GIL and attach
    Py_BEGIN_ALLOW_THREADS;                                
    ok = base_bp_attach();
    Py_END_ALLOW_THREADS   
    
    // Error handling
    if (ok < 0) {
        pyion_SetExc(PyExc_SystemError, "Cannot attach to BP engine. Is ION running on this host?");
        return NULL;
    }

    // Return True to indicate success
    Py_RETURN_TRUE;
}

static PyObject *pyion_bp_detach(PyObject *self, PyObject *args) {
    // Initialize variables
    int ok;

    // Release GIL and detach
    Py_BEGIN_ALLOW_THREADS                                
    ok = base_bp_detach();
    Py_END_ALLOW_THREADS

    // Error handling
    if (ok < 0) {
        pyion_SetExc(PyExc_SystemError, "Cannot attach to BP engine. Is ION running on this host?");
        return NULL;
    }

    // Return True to indicate success
    Py_RETURN_TRUE;
}

/* ============================================================================
 * === Open/Close Endpoint Functions
 * ============================================================================ */

static PyObject *pyion_bp_open(PyObject *self, PyObject *args) {
    // Initialize variables
    BpSapState *state = NULL;
    int ok;
    char *ownEid;
    int detained, mem_ctrl;

     // Parse the input tuple. Raises error automatically if not possible
    if (!PyArg_ParseTuple(args, "sii", &ownEid, &detained, &mem_ctrl))
        return NULL;
    
    // Release GIL and open 
    Py_BEGIN_ALLOW_THREADS                                
    ok = base_bp_open(&state, ownEid, detained, mem_ctrl);
    Py_END_ALLOW_THREADS

    // Allocate memory for state and initialize to zeros
    switch(ok) {
        case -1:
            pyion_SetExc(PyExc_RuntimeError, "Cannot malloc for BP state.");
            break;
        case -2:
            pyion_SetExc(PyExc_ConnectionError, "Cannot open endpoint '%s'. Is it defined in .bprc? Is it already in use?", ownEid);
            break;
        case -3:
            pyion_SetExc(PyExc_RuntimeError, "Can't initialize memory attendant.");
            break;
    }

    // If error, return
    if (ok < PYION_OK) return NULL;

    // Return the memory address of the SAP for this endpoint as an unsined long
    PyObject *ret = Py_BuildValue("k", state);

    return ret;
}

static PyObject *pyion_bp_close(PyObject *self, PyObject *args) {
    // Define variables
    BpSapState *state;

    // Parse the input tuple. Raises error automatically if not possible
    if (!PyArg_ParseTuple(args, "k", (unsigned long *)&state))
        return NULL;

    // If endpoint is in idle state, just close
    if (state->status == EID_IDLE) {
        Py_BEGIN_ALLOW_THREADS 
        base_close_endpoint(state);
        Py_END_ALLOW_THREADS

        Py_RETURN_NONE;
    }

    // We assume that if you reach this point, you are always in 
    // running state.
    state->status = EID_CLOSING;
    bp_interrupt(state->sap);
    
    Py_RETURN_NONE;
}

/* ============================================================================
 * === Interrupt Endpoint Functions
 * ============================================================================ */

static PyObject *pyion_bp_interrupt(PyObject *self, PyObject *args) {
    // Define variables
    BpSapState *state;

    // Parse the input tuple. Raises error automatically if not possible
    if (!PyArg_ParseTuple(args, "k", (unsigned long *)&state))
        return NULL;

    // If EID is not running, you do not need to interrupt
    if (state->status != EID_RUNNING)
        Py_RETURN_NONE;

    // Mark that you have transitioned to interruping state
    state->status = EID_INTERRUPTING;
    
    // Intrrupt the endpoint
    Py_BEGIN_ALLOW_THREADS    
    base_bp_interrupt(state);
    Py_END_ALLOW_THREADS

    Py_RETURN_NONE;
}

/* ============================================================================
 * === Send Functionality
 * ============================================================================ */

static PyObject *pyion_bp_send(PyObject *self, PyObject *args) {
    // Define variables
    Sdr sdr = NULL;
    int data_size;
    char *data;
    char *reportEid;
    char *destEid = NULL;
    int ok, ttl, classOfService, rrFlags, ackReq;
    unsigned int retxTimer;
    BpCustodySwitch custodySwitch;
    BpAncillaryData *ancillaryData = NULL;
    BpSapState *state = NULL;
    TxPayload txInfo;
    int status; 

    // Parse input arguments. First one is SAP memory address for this endpoint
    if (!PyArg_ParseTuple(args, "ksziiiiiIs#", (unsigned long *)&state, &destEid, &reportEid, &ttl,
                          &classOfService, (int *)&custodySwitch, &rrFlags, &ackReq, &retxTimer,
                          &data, &data_size))
        return NULL;

    // Initialize TxPayload struct
    txInfo.destEid = malloc(sizeof(char)*(strlen(destEid)+1));   
    txInfo.reportEid = (reportEid == NULL) ? NULL : malloc(sizeof(char)*(strlen(reportEid)+1));
    txInfo.data = malloc(sizeof(char)*data_size);

    // Create TxPayload struct
    strcpy(txInfo.destEid, destEid);
    if (reportEid) strcpy(txInfo.reportEid, reportEid);
    txInfo.ttl = ttl;
    txInfo.classOfService = classOfService;
    txInfo.custodySwitch = custodySwitch;
    txInfo.rrFlags = rrFlags;
    txInfo.ackReq = ackReq;
    txInfo.retxTimer = retxTimer;
    strcpy(txInfo.data, data);
    txInfo.data_size = data_size;
    txInfo.ancillaryData = ancillaryData;

    // Release the GIL
    Py_BEGIN_ALLOW_THREADS                                
    status = base_bp_send(state, &txInfo);                        
    Py_END_ALLOW_THREADS

    // Free memory
    free(txInfo.destEid);
    if (reportEid) free(txInfo.reportEid);
    free(txInfo.data);

    // Error Handling
    switch(status) {
        case PYION_OK: 
            Py_RETURN_TRUE;
            break;
        case PYION_SDR_ERR: 
            pyion_SetExc(PyExc_MemoryError, "SDR memory could not be allocated.");
            break;
        case PYION_ZCO_ERR:
            pyion_SetExc(PyExc_MemoryError, "ZCO object creation failed.");
            break;
        case PYION_IO_ERR:
            pyion_SetExc(PyExc_RuntimeError, "Error while sending the bundle.");
            break;
        case PYION_ERR:
            pyion_SetExc(PyExc_RuntimeError, "Error while scheduling custodial retransmission.");
            break;
        default:
            pyion_SetExc(PyExc_RuntimeError, "Error while sending the bundle.");
    }

    // If you reach this point, return NULL to indicate exception
    return NULL;
}

/* ============================================================================
 * === Receive Functionality
 * ============================================================================ */

static PyObject *pyion_bp_receive(PyObject *self, PyObject *args) {
    // Define variables
    BpSapState *state;
    PyObject *ret;
    int status;
    RxPayload msg = {NULL, 0, 0};
    msg.payload = msg.payload_prealloc;

    // Parse the input tuple. Raises error automatically if not possible
    if (!PyArg_ParseTuple(args, "k", (unsigned long *)&state))
        return NULL;

    // Mark as running
    state->status = EID_RUNNING;

    // Release GIL and received data
    Py_BEGIN_ALLOW_THREADS                                
    status = base_bp_receive_data(state, &msg);
    Py_END_ALLOW_THREADS

    // Error handling
    switch(status) {
        case PYION_IO_ERR:
            pyion_SetExc(PyExc_IOError, "Error receiving bundle through endpoint.");
            break;
        case PYION_INTERRUPTED_ERR:
            pyion_SetExc(PyExc_InterruptedError, "BP reception interrupted.");
            break;
        case PYION_CONN_ABORTED_ERR:
            pyion_SetExc(PyExc_ConnectionAbortedError, "BP reception closed.");
            break;
        case PYION_SDR_ERR:
            pyion_SetExc(PyExc_MemoryError, "SDR memory could not be read.");
            break;
    }

    // If error, return NULL
    if (status < PYION_OK) return NULL;

    // Build return object
    ret = Py_BuildValue("y#", msg.payload, msg.len);

    // If you allocated memory for this payload, free it here
    if (msg.do_malloc) free(msg.payload);

    // Return value
    return ret;
}

/* ============================================================================
 * === EOF
 * ============================================================================ */