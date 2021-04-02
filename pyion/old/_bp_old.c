/* ============================================================================
 * Extension module to interface ION and Python. It works as follows:
 *  - Each opened endpoint creates a BpSapState object. From then onwards, to
 *    send/receive through that endpoint, you need to use this state object
 *    (i.e., there is a one-to-one mapping between endpoint and BpSapState).
 *  - After opening an endpoint, the memory address of the BpSapState object is
 *    returned to Python as an ``unsigned long``. The ``Proxy`` in ``pyion.py``
 *    is then responsible for storing that reference in a Python dictionary of
 *    the form {'ipn:1.1':mem_addr_1, 'ipn:1.2':mem_addr_2, ...}.
 *  - To send/receive from and endpoint, the ``Endpoint`` object in ``pyion.py``
 *    code sends ``mem_addr_i`` as an argument to the C function, that then 
 *    can retrieve the appropriate BpSapState object for the originating endpoint.
 *  - To close and endpoint, the ``Proxy`` class in ``pyion.py`` provides 
 *    the mem_addr of the BpSapState to the corresponding C function, which 
 *    then frees the memory.
 * 
 *  .. Warning:: If the ``Proxy`` object loses the BpSapState memory addresses,
 *               there is now way to retrieve them from C. Therefore, you will
 *               have a memory leak.
 * 
 * Handling SIGINT Signals
 * -----------------------
 * ``bp_send`` is not concerned with this discussion. According to Scott Burleigh,
 * it is always a non-blocking call.
 * 
 * ``bp_receive`` blocks until some data is received. Therefore, ideally you want 
 * to have the ability to stop it at the user's request by issuing a SIGINT. This
 * is achieved from ``pyion.c`` as opposed to doing it directly in the C extension.
 * Also, to ensure no race conditions, the BpSapState object tracks the endpoint
 * status, which can be either EID_IDLE, EID_RUNNING, EID_CLOSING, EID_INTERRUPTING.
 * 
 * .. Warning:: While the status in BpSapState prevents most race conditions, 
 *              some edge cases might still be possible (e.g., the user issues
 *              a SIGINT while you are closing a endpoint).
 *
 * Limitations
 * -----------
 * The ancillary data of a bundle (Extension blocks, etc.) is currently not 
 * implemented.
 * 
 * Author: Marc Sanchez Net
 * Date:   4/15/2019
 * Copyright (c) 2019, California Institute of Technology ("Caltech").  
 * U.S. Government sponsorship acknowledged.
 * =========================================================================== */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <bp.h>
#include <Python.h>

#include "_utils.c"

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
 * === Define structures for this module
 * ============================================================================ */

// Possible states of an enpoint. This is used to avoid race conditions
// when closing receiving threads.
typedef enum {
    EID_IDLE = 0,
    EID_RUNNING,
    EID_CLOSING,
    EID_INTERRUPTING
} SapStateEnum;

// A combination of a BpSAP object and a representation of its status.
// The status only used during reception for now.
typedef struct {
    BpSAP sap;
    SapStateEnum status;
    int detained;
    ReqAttendant* attendant;
} BpSapState;

/* ============================================================================
 * === Define global variables
 * ============================================================================ */

#define MAX_PREALLOC_BUFFER 1024

/* ============================================================================
 * === Attach/Detach Functions
 * ============================================================================ */

static PyObject *pyion_bp_attach(PyObject *self, PyObject *args) {
    // Try to attach to BP agent
    if (bp_attach() < 0) {
        pyion_SetExc(PyExc_SystemError, "Cannot attach to BP engine. Is ION running on this host?");
        return NULL;
    }

    // Return True to indicate success
    Py_RETURN_TRUE;
}

static PyObject *pyion_bp_detach(PyObject *self, PyObject *args) {
    // Detach from BP agent
    bp_detach();

    // Return True to indicate success
    Py_RETURN_TRUE;
}

/* ============================================================================
 * === Open/Close Endpoint Functions
 * ============================================================================ */

static PyObject *pyion_bp_open(PyObject *self, PyObject *args) {
    // Define variables
    char *ownEid;
    int detained, ok, mem_ctrl;

    // Allocate memory for state and initialize to zeros
    BpSapState *state = (BpSapState*)malloc(sizeof(BpSapState));
    if (state == NULL) {
        pyion_SetExc(PyExc_RuntimeError, "Cannot malloc for BP state.");
        return NULL;
    }

    // Set memory contents to zeros
    memset((char *)state, 0, sizeof(BpSapState));

    // Parse the input tuple. Raises error automatically if not possible
    if (!PyArg_ParseTuple(args, "sii", &ownEid, &detained, &mem_ctrl))
        return NULL;
    
    // Open the endpoint. This call fills out the SAP information
    // NOTE: An endpoint must be opened in detained mode if you want
    //       to have custody-based re-tx.
    if (detained == 0) {
        ok = bp_open(ownEid, &(state->sap));
    } else {
        ok = bp_open_source(ownEid, &(state->sap), 1);
    }

    // Handle error while opening endpoint
    if (ok < 0) {
        pyion_SetExc(PyExc_ConnectionError, "Cannot open endpoint '%s'. Is it defined in .bprc? Is it already in use?", ownEid);
        return NULL;
    }

    // Mark the SAP state for this endpoint as running
    state->status   = EID_IDLE;
    state->detained = (detained > 0);

    // If you need ZCO memory control, create an attendant
    if (mem_ctrl) {
        // Allocate memory for new attendant
        state->attendant = (ReqAttendant*)malloc(sizeof(ReqAttendant));
        
        // Initialize the attendant
        if (ionStartAttendant(state->attendant)) {
            pyion_SetExc(PyExc_RuntimeError, "Can't initialize memory attendant.");
            return NULL;
        }
    } else {
        state->attendant = NULL;
    }

    // Return the memory address of the SAP for this endpoint as an unsined long
    PyObject *ret = Py_BuildValue("k", state);
    return ret;
}

static void close_endpoint(BpSapState *state) {
    // Close and free attendant
    if (state->attendant) {
        ionStopAttendant(state->attendant);
        free(state->attendant);
    } 

    // Close this SAP
    bp_close(state->sap);

    // Free state memory
    free(state);
}

static PyObject *pyion_bp_close(PyObject *self, PyObject *args) {
    // Define variables
    BpSapState *state;

    // Parse the input tuple. Raises error automatically if not possible
    if (!PyArg_ParseTuple(args, "k", (unsigned long *)&state))
        return NULL;

    // If endpoint is in idle state, just close
    if (state->status == EID_IDLE) {
        close_endpoint(state);
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
    bp_interrupt(state->sap);

    // Pause the attendant
    if (state->attendant) ionPauseAttendant(state->attendant);

    Py_RETURN_NONE;
}

/* ============================================================================
 * === Send Functionality
 * ============================================================================ */

static PyObject *pyion_bp_send(PyObject *self, PyObject *args) {
    // Define variables
    char *destEid = NULL;
    char *reportEid = NULL;
    Sdr sdr = NULL;
    const char *data = NULL;
    int data_size;
    Object bundleSdr;
    Object bundleZco;
    Object newBundle;
    int ok, ttl, classOfService, rrFlags, ackReq;
    unsigned int retxTimer;
    BpCustodySwitch custodySwitch;
    BpAncillaryData *ancillaryData = NULL;
    BpSapState *state = NULL;

    // Parse input arguments. First one is SAP memory address for this endpoint
    if (!PyArg_ParseTuple(args, "ksziiiiiIs#", (unsigned long *)&state, &destEid, &reportEid, &ttl,
                          &classOfService, (int *)&custodySwitch, &rrFlags, &ackReq, &retxTimer,
                          &data, &data_size))
        return NULL;

    // Initialize variables
    sdr = bp_get_sdr();

    // Insert data to SDR
    if (!sdr_pybegin_xn(sdr)) return NULL;
    bundleSdr = sdr_insert(sdr, data, (size_t)data_size);
    if (!sdr_pyend_xn(sdr)) return NULL;

    // If insert failed, cancel transaction and exit
    if (!bundleSdr) {
        pyion_SetExc(PyExc_MemoryError, "SDR memory could not be allocated.");
        return NULL;
    }

    // Create the ZCO object. If not enough ZCO space, and the attendant is not NULL, then
    // this is a blocking call. Therefore, release the GIL
    Py_BEGIN_ALLOW_THREADS                                
    bundleZco = ionCreateZco(ZcoSdrSource, bundleSdr, 0, data_size,
                             classOfService, 0, ZcoOutbound, state->attendant);                                        
    Py_END_ALLOW_THREADS

    // Handle error while creating ZCO object
    if (!bundleZco || bundleZco == (Object)ERROR) {
        pyion_SetExc(PyExc_MemoryError, "ZCO object creation failed.");
        return NULL;
    }

    // Send ZCO object using BP protocol.  
    Py_BEGIN_ALLOW_THREADS                           
    ok = bp_send(state->sap, destEid, reportEid, ttl, classOfService, custodySwitch,
                 rrFlags, ackReq, ancillaryData, bundleZco, &newBundle);
    Py_END_ALLOW_THREADS

    // Handle error in bp_send
    if (ok <= 0) {
        pyion_SetExc(PyExc_RuntimeError, "Error while sending the bundle (err code=%i).", ok);
        return NULL;
    }

    // If you want custody transfer and have specified a re-transmission timer,
    // then activate it
    if (custodySwitch == SourceCustodyRequired && retxTimer > 0) {
        // Note: The timer starts as soon as bp_memo is called.
        ok = bp_memo(newBundle, retxTimer);

        // Handle error in bp_memo
        if (ok < 0) {
            pyion_SetExc(PyExc_RuntimeError, "Error while scheduling custodial retransmission (err code=%i).", ok);
            return NULL;
        }
    }

    // If you have opened this endpoint in detained mode, you need to release the bundle
    if (state->detained) bp_release(newBundle);

    // Return True to indicate success
    Py_RETURN_TRUE;
}

/* ============================================================================
 * === Receive Functionality
 * ============================================================================ */

static PyObject *receive_data(BpSapState *state, BpDelivery *dlv){
    // Define variables
    int data_size, len, rx_ret, do_malloc;
    Sdr sdr;
    ZcoReader reader;

    // Define variables to store the bundle payload. If payload size is less than
    // MAX_PREALLOC_BUFFER, then use preallocated buffer to save time. Otherwise,
    // call malloc to allocate as much memory as you need.
    char prealloc_payload[MAX_PREALLOC_BUFFER];
    char *payload;

    // Get ION's SDR
    sdr = bp_get_sdr();

    while (state->status == EID_RUNNING) {
        // Receive the next bundle. This is a blocking call. Therefore, release the GIL
        Py_BEGIN_ALLOW_THREADS                                // Release the GIL
        rx_ret = bp_receive(state->sap, dlv, BP_BLOCKING);
        Py_END_ALLOW_THREADS                                  // Acquire the GIL

        // Check if error while receiving a bundle
        if ((rx_ret < 0) && (state->status == EID_RUNNING)) {
            pyion_SetExc(PyExc_IOError, "Error receiving bundle through endpoint (err code=%d).", rx_ret);
            return NULL;
        }

        // If dlv is not interrupted (e.g., it was successful), get out of loop.
        // From Scott Burleigh: BpReceptionInterrupted can happen because SO triggers an
        // interruption without the user doing anything. Therefore, bp_receive always
        // needs to be enclosed in this type of while loops.
        if (dlv->result != BpReceptionInterrupted)
            break;
    }

    // If you exited because of interruption
    if (state->status == EID_INTERRUPTING) {
        pyion_SetExc(PyExc_InterruptedError, "BP reception interrupted.");
        return NULL;
    }

    // If you exited because of closing
    if (state->status == EID_CLOSING) {
        pyion_SetExc(PyExc_ConnectionAbortedError, "BP reception closed.");
        return NULL;
    }

    // If endpoint was stopped, finish
    if (dlv->result == BpEndpointStopped) {
        pyion_SetExc(PyExc_ConnectionAbortedError, "BP endpoint was stopped.");
        return NULL;
    }

    // If bundle does not have the payload, raise IOError
    if (dlv->result != BpPayloadPresent) {
        pyion_SetExc(PyExc_IOError, "Bundle received without payload.");
        return NULL;
    }

    // Get content data size
    if (!sdr_pybegin_xn(sdr)) return NULL;
    data_size = zco_source_data_length(sdr, dlv->adu);
    sdr_pyexit_xn(sdr);

    // Check if we need to allocate memory dynamically
    do_malloc = (data_size > MAX_PREALLOC_BUFFER);

    // Allocate memory if necessary
    payload = do_malloc ? (char *)malloc(data_size) : prealloc_payload;

    // Initialize reader
    zco_start_receiving(dlv->adu, &reader);

    // Get bundle data
    if (!sdr_pybegin_xn(sdr)) return NULL;
    len = zco_receive_source(sdr, &reader, data_size, payload);

    // Handle error while getting the payload
    if (sdr_end_xn(sdr) < 0 || len < 0) {
        pyion_SetExc(PyExc_IOError, "Error extracting payload from bundle.");

        // Clean up tasks
        if (do_malloc) free(payload);
        return NULL;
    }

    // Build return object
    PyObject *ret = Py_BuildValue("y#", payload, len);

    // If you allocated memory for this payload, free it here
    if (do_malloc) free(payload);

    return ret;
}

static PyObject *pyion_bp_receive(PyObject *self, PyObject *args) {
    // Define variables
    BpSapState *state;
    PyObject *ret;
    BpDelivery dlv;

    // Parse the input tuple. Raises error automatically if not possible
    if (!PyArg_ParseTuple(args, "k", (unsigned long *)&state))
        return NULL;

    // Mark as running
    state->status = EID_RUNNING;

    // Trigger reception of data
    ret = receive_data(state, &dlv);

    // Clean up tasks
    bp_release_delivery(&dlv, 1);

    // Close if necessary. Otherwise set to IDLE
    if (state->status == EID_CLOSING) {
        close_endpoint(state);
    } else {
        state->status = EID_IDLE;
    }

    // Return value
    return ret;
}

/* ============================================================================
 * === BP Report Parsing functionality
 * ============================================================================ */

// TODO