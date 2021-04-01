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
 * === Define global variables
 * ============================================================================ */

#define MAX_LTP_SESSIONS 1024

/* ============================================================================
 * === Define structures for this module
 * ============================================================================ */


/* ============================================================================
 * === Attach/Detach Functions
 * ============================================================================ */

static PyObject *pyion_ltp_attach(PyObject *self, PyObject *args) {
    // Define variables
    char err_msg[150];
    // Try to attach to BP agent
    if (base_ltp_attach() < 0) {
        sprintf(err_msg, "Cannot attach to LTP engine. Is ION running on this host?");
        PyErr_SetString(PyExc_SystemError, err_msg);
        return NULL;
    }
    Py_RETURN_NONE;
}

static PyObject *pyion_ltp_detach(PyObject *self, PyObject *args) {
    // Detach from BP agent
    base_ltp_detach();
    Py_RETURN_NONE;
}

/* ============================================================================
 * === Open/Close Endpoint Functions
 * ============================================================================ */

static PyObject *pyion_ltp_open(PyObject *self, PyObject *args) {
    // Define variables
    unsigned int clientId;
    char err_msg[150];

    LtpSAP *state = NULL;

    

    // Parse the input tuple. Raises error automatically if not possible
    if (!PyArg_ParseTuple(args, "I", &clientId))
        return NULL;

    // Open connection to LTP client

    int open_status = base_ltp_open(clientId, &state);

    if (open_status < 0) {
        switch (open_status) {
            case PYION_MALLOC_ERR:
                sprintf(err_msg, "Failed to malloc for LtpSapState");
                PyErr_SetString(PyExc_MemoryError, err_msg);
                return NULL;
            default:
                sprintf(err_msg, "Cannot open LTP client access point.");
                PyErr_SetString(PyExc_RuntimeError, err_msg);
                 return NULL;
        }
    }

   

    // Return the memory address of the LTP state as an unsined long
    PyObject *ret = Py_BuildValue("k", state);
    return ret;
}



static PyObject *pyion_ltp_close(PyObject *self, PyObject *args) {
    // Define variables
    LtpSAP *state;

    // Parse the input tuple. Raises error automatically if not possible
    if (!PyArg_ParseTuple(args, "k", (unsigned long *)&state))
        return NULL;

    // If endpoint is in idle state, just close
    if (state->status == SAP_IDLE) {
        base_ltp_close(state);
        Py_RETURN_NONE;
    }

    // We assume that if you reach this point, you are always in 
    // running state.
    state->status = SAP_CLOSING;
    ltp_interrupt(state->clientId);
    
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

    // If access point is already closing, you can return
    if (state->status != SAP_RUNNING)
        Py_RETURN_NONE;

    // Mark that you have transitioned to interruping state
    base_ltp_interrupt(state);

    Py_RETURN_NONE;
}

/* ============================================================================
 * === Send Functionality
 * ============================================================================ */

static PyObject *pyion_ltp_send(PyObject *self, PyObject *args) {
    // Define variables
    char                err_msg[150];
    LtpSAP              *state;
    int ok;
    LtpTxPayload        txInfo;

    // Parse input arguments. First one is SAP memory address for this endpoint
    if (!PyArg_ParseTuple(args, "kKs#", (unsigned long *)&state, &(txInfo.destEngineId),
     &(txInfo.data), &(txInfo.data_size)))
        return NULL;

   

    // Send using LTP protocol. All data is sent as RED LTP by definition.
    // NOTE 1: SessionId is filled by ``ltp_send``, but you do not care about it
    // NOTE 2: In general, ltp_send does not block. However, if you exceed the max
    //         number of export sessions defined in ltprc, then it will.
    Py_BEGIN_ALLOW_THREADS
    ok = base_ltp_send(state, &txInfo);
    Py_END_ALLOW_THREADS

    // Handle error in ltp_send
    if (ok <= 0) {
        sprintf(err_msg, "Error while sending the data through LTP (err code=%i)", ok);
        PyErr_SetString(PyExc_RuntimeError, err_msg);
        return NULL;
    }
    
    Py_RETURN_NONE;
}

/* ============================================================================
 * === Receive Functionality
 * ============================================================================ */

static int base_ltp_receive_data(LtpSAP *state, LtpRxPayload *payloadObj){
    // Define variables
    char            err_msg[150];
    ZcoReader	    reader;
    Sdr             sdr;
    LtpNoticeType	type;
	LtpSessionId	sessionId;
	unsigned char	reasonCode;
	unsigned char	endOfBlock;
	unsigned int	dataOffset;
	unsigned int	dataLength;
	Object		    data;
    int             receiving_block, notice, do_malloc;
    vast            len, data_size;

    // Create a pre-allocated buffer for small block sizes. Otherwise, use malloc to
    // get dynamic memory
    

    // Mark as running
    receiving_block = 1;
    state->status = SAP_RUNNING;

    // Process incoming indications
    while ((state->status == SAP_RUNNING) && (receiving_block == 1)) {
        // Get the next LTP notice
        notice = ltp_get_notice(state->clientId, &type, &sessionId, &reasonCode, 
                                &endOfBlock, &dataOffset, &dataLength, &data);

        // Handle error while receiving notices
        if (notice < 0) {
            //Need to set error string receiving notices.
            return NULL;
        }

        // Handle different notice types
        switch (type) {
            case LtpExportSessionComplete:      // Transmit success
                // Do not mark reception of block complete here. This should be done by the 
                // LtpRecvRedPart case.
                break;
            case LtpImportSessionCanceled:      // Cancelled sessions. No data has been received yet.
                // Release any data and throw exception
                ltp_release_data(data);
                sprintf(err_msg, "LTP import session cancelled (reason code=%d)", (unsigned int)reasonCode);
                PyErr_SetString(PyExc_RuntimeError, err_msg);
                return NULL;
            case LtpExportSessionCanceled:      // Transmit failure. Some data might have been received already.
                // Release any data and throw exception
                ltp_release_data(data);
                sprintf(err_msg, "LTP export session cancelled (reason code=%d)", (unsigned int)reasonCode);
                PyErr_SetString(PyExc_RuntimeError, err_msg);
                return NULL;
            case LtpRecvRedPart:
                // If this is not the end of the block, you are dealing with a block that is
                // partially green, partially red. This is not allowed.
                if (!endOfBlock) {
                    ltp_release_data(data);
                    PyErr_SetString(PyExc_NotImplementedError, "An LTP block cannot have green parts");
                    return NULL;
                }
                
                // Mark that reception of block has ended
                receiving_block = 0;
                break;
            case LtpRecvGreenSegment:
                // Release any data and throw exception
                ltp_release_data(data);
                PyErr_SetString(PyExc_NotImplementedError, "Only red part of LTP is supported");
                return NULL;
            default:
                break;
        }

        // Make sure other tasks have a chance to run
        sm_TaskYield();
    }

    // If you exited because of closing, throw error
    if (state->status == SAP_CLOSING) {
        //PyErr_SetString(PyExc_ConnectionAbortedError, "LTP reception closed.");
        return NULL;
    }

    // If no block received by now and you are not closing, error.
    if (receiving_block == -1) {
        //PyErr_SetString(PyExc_RuntimeError, "LTP block was not delivered as expected.");
        return NULL;
    }

    // Get ION SDR
    sdr = getIonsdr();

    // Get content data size
    if (!sdr_pybegin_xn(sdr)) return NULL;
    data_size = zco_source_data_length(sdr, data);
    sdr_exit_xn(sdr);

    // Check if we need to allocate memory dynamically
    do_malloc = 1;

    // Allocate memory for payload
    payloadObj->payload =  (char*)malloc(data_size);

    // Prepare to receive next block
    zco_start_receiving(data, &reader);

    // Get bundle data
    if (!sdr_pybegin_xn(sdr)) return NULL;
    payloadObj->len = zco_receive_source(sdr, &reader, data_size, payloadObj->payload);
    if (!sdr_pyend_xn(sdr)) return NULL;


    // Handle error while getting the payload
    if (len < 0) {
        sprintf(err_msg, "Error extracting data from block");
        //PyErr_SetString(PyExc_IOError, err_msg);

        // Clean up tasks
        
        return NULL;
    }

    // Build return object
   

    // Release LTP object now that you are done with it.
    ltp_release_data(data);

    // Deallocate memory
    if (do_malloc) free(payloadObj->payload);

    return 0;
}

static PyObject *pyion_ltp_receive(PyObject *self, PyObject *args) {
    // Define variables
    LtpSAP   *state;
    LtpRxPayload *payloadObj;
    int ok;
    
    // Parse the input tuple. Raises error automatically if not possible
    if (!PyArg_ParseTuple(args, "k", (unsigned long *)&state))
        return NULL;

    // Trigger reception of data
    ok = base_ltp_receive_data(state, payloadObj);

    // Close if necessary. Otherwise set to IDLE
    if (state->status == SAP_CLOSING) {
       base_ltp_close(state);
    } else {
        state->status = SAP_IDLE;
    }

    PyObject *ret = Py_BuildValue("y#", payloadObj->payload, payloadObj->len);

    free(payloadObj->payload);

    // Return value
    return ret;
}