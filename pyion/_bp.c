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
#include "base_bp.h"

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
    "Long [k]: Memory address of SAP to receive from\n"
    "Int [i]: Return bundle headers as dictionary. Defaults to false.";
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
    {NULL, NULL, 0, NULL}};

/* ============================================================================
 * === Define _bp as a Python module
 * ============================================================================ */

PyMODINIT_FUNC PyInit__bp(void)
{
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
    if (!module)
        return NULL;

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
 * === Define global variables
 * ============================================================================ */

#define MAX_PREALLOC_BUFFER 1024

/* ============================================================================
 * === Attach/Detach Functions
 * =================================s=========================================== */

static PyObject *pyion_bp_attach(PyObject *self, PyObject *args)
{
    // Try to attach to BP agent
    int oK;
    Py_BEGIN_ALLOW_THREADS;
    oK = base_bp_attach();
    Py_END_ALLOW_THREADS if (oK < 0)
    {
        pyion_SetExc(PyExc_SystemError, "Cannot attach to BP engine. Is ION running on this host?");
        return NULL;
    }

    // Return True to indicate success
    Py_RETURN_TRUE;
}

static PyObject *pyion_bp_detach(PyObject *self, PyObject *args)
{
    Py_BEGIN_ALLOW_THREADS
    //bp_detach is a void function. If it fails, then something
    //serious is happening and will kill the process as necessary.
    base_bp_detach();
    Py_END_ALLOW_THREADS

    // Return True to indicate success
    Py_RETURN_TRUE;
}

/* ============================================================================
 * === Open/Close Endpoint Functions
 * ============================================================================ */

static PyObject *pyion_bp_open(PyObject *self, PyObject *args)
{
    // State object, we will be returning a form of this
    BpSapState *state = NULL;
    int ok;
    char *ownEid;
    int detained, mem_ctrl;

    // Parse the input tuple. Raises error automatically if not possible
    if (!PyArg_ParseTuple(args, "sii", &ownEid, &detained, &mem_ctrl))
        return NULL;

    ok = base_bp_open(&state, ownEid, detained, mem_ctrl);
    // Allocate memory for state and initialize to zeros
    if (ok == -1)
    {
        pyion_SetExc(PyExc_RuntimeError, "Cannot malloc for BP state.");
        return NULL;
    }

    // Set memory contents to zeros

    // Handle error while opening endpoint
    if (ok == -2)
    {
        pyion_SetExc(PyExc_ConnectionError, "Cannot open endpoint '%s'. Is it defined in .bprc? Is it already in use?", ownEid);
        return NULL;
    }

    if (ok == -3)
    {
        pyion_SetExc(PyExc_RuntimeError, "Can't initialize memory attendant.");
        return NULL;
    }

    // Return the memory address of the SAP for this endpoint as an unsined long
    PyObject *ret = Py_BuildValue("k", state);
    return ret;
}

static PyObject *pyion_bp_close(PyObject *self, PyObject *args)
{
    // Define variables
    BpSapState *state;

    // Parse the input tuple. Raises error automatically if not possible
    if (!PyArg_ParseTuple(args, "k", (unsigned long *)&state))
        return NULL;

    // If endpoint is in idle state, just close
    if (state->status == EID_IDLE)
    {
        base_close_endpoint(state);
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

static PyObject *pyion_bp_interrupt(PyObject *self, PyObject *args)
{
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
    base_bp_interrupt(state);

    Py_RETURN_NONE;
}

/* ============================================================================
 * === Send Functionality
 * ============================================================================ */

static PyObject *pyion_bp_send(PyObject *self, PyObject *args)
{
    // Define variables

    Sdr sdr = NULL;
    Py_ssize_t data_size;
    char *data;
    char *reportEid, *destEid;
    int ok, ttl, classOfService, rrFlags, ackReq;
    unsigned int retxTimer;
    BpCustodySwitch custodySwitch;
    BpAncillaryData *ancillaryData = NULL;
    BpSapState *state = NULL;
    BpTx txInfo;
    int status; //return status of bp_send

    // Parse input arguments. First one is SAP memory address for this endpoint
    if (!PyArg_ParseTuple(args, "ksziiiiiIs#", (unsigned long *)&state, &destEid, &reportEid, &ttl,
                          &classOfService, (int *)&custodySwitch, &rrFlags, &ackReq, &retxTimer,
                          &data, &data_size))
        return NULL;

    // Create TxPayload struct
    base_init_bp_tx_payload(&txInfo);
    txInfo.destEid = destEid;
    txInfo.reportEid = reportEid;
    txInfo.ttl = ttl;
    txInfo.classOfService = classOfService;
    txInfo.custodySwitch = custodySwitch;
    txInfo.rrFlags = rrFlags;
    txInfo.ackReq = ackReq;
    txInfo.retxTimer = retxTimer;
    txInfo.data = data;
    txInfo.data_size = (int)data_size;   // Dangerous cast
    txInfo.ancillaryData = ancillaryData;

    // Release the GIL
    Py_BEGIN_ALLOW_THREADS
    status = base_bp_send(state, &txInfo);
    Py_END_ALLOW_THREADS

    switch (status)
    {
    case 0:
        // Return True to indicate success
        Py_RETURN_TRUE;
        break;
    case PYION_SDR_ERR:
        pyion_SetExc(PyExc_MemoryError, "SDR memory could not be allocated.");
        return NULL;
    case PYION_IO_ERR:
        pyion_SetExc(PyExc_MemoryError, "ZCO object creation failed.");
        return NULL;
    case 3:
        pyion_SetExc(PyExc_RuntimeError, "Error while scheduling custodial retransmission (err code=%i).", 3);
        return NULL;
    default:
        pyion_SetExc(PyExc_RuntimeError, "Error while sending the bundle (err code=%i).", status);
        return NULL;
    }
}

/* ============================================================================
 * === Receive Functionality
 * ============================================================================ */

static PyObject *pyion_bp_receive(PyObject *self, PyObject *args)
{
    // Define variables
    BpSapState *state;
    PyObject *ret_payload;
    PyObject *ret_payload_metadata;
    PyObject *ret;
    int status;
    BpRx msg;

    int return_header = 0; // Don't return header by default
    
    // Initialize output structure
    base_init_bp_rx_payload(&msg);

    // Parse the input tuple. Raises error automatically if not possible
    if (!PyArg_ParseTuple(args, "ki", (unsigned long *)&state, &return_header))
        return NULL;

    // Mark as running
    state->status = EID_RUNNING;

    Py_BEGIN_ALLOW_THREADS // Release the GIL
    status = base_bp_receive_data(state, &msg);
    Py_END_ALLOW_THREADS

    // If an error occurred, free our memory to prevent a leak.
    if (status)
    { 
        if (msg.do_malloc)
            free(msg.payload);
    }

    // Throw different exceptions depending on error code
    switch (status)
    {
    case PYION_INTERRUPTED_ERR:
        pyion_SetExc(PyExc_ConnectionError, "ION Connection Interrupted");
        return NULL;
    case PYION_CONN_ABORTED_ERR:
        pyion_SetExc(PyExc_ConnectionError, "ION Connection aborted error.");
        return NULL;
    case PYION_IO_ERR:
        pyion_SetExc(PyExc_IOError, "ION IO Error");
        return NULL;
    case PYION_SDR_ERR:
        pyion_SetExc(PyExc_MemoryError, "SDR Failure");
        return NULL;
    }

    // Build first object (message payload) in return tuple
    ret_payload = Py_BuildValue("y#", msg.payload, (Py_ssize_t)msg.len);
    
    if (return_header){
        // Build second object (message metadata) in return tuple
        ret_payload_metadata = PyDict_New();

        // Add some key-value pairs to the dictionary (modify as needed)
        PyDict_SetItemString(ret_payload_metadata, "timeToLive", PyLong_FromLong(msg.timeToLive));
        PyDict_SetItemString(ret_payload_metadata, "bundleSourceEid", Py_BuildValue("s", msg.bundleSourceEid));
        PyDict_SetItemString(ret_payload_metadata, "metadata", Py_BuildValue("y#", msg.metadata, (Py_ssize_t)msg.metadataLen));
        PyDict_SetItemString(ret_payload_metadata, "metadataType", Py_BuildValue("c", msg.metadataType));
        PyDict_SetItemString(ret_payload_metadata, "bundleCreationTimeCount", PyLong_FromLong(msg.bundleCreationTime.count));
        PyDict_SetItemString(ret_payload_metadata, "bundleCreationTimeMsec", PyLong_FromUnsignedLongLong(msg.bundleCreationTime.msec));

        // Build return object
        ret = PyTuple_Pack(2, ret_payload, ret_payload_metadata);
    } else {
        ret = ret_payload;
    }

    
    // If you allocated memory for this payload, free it here
    if (msg.do_malloc)
        free(msg.payload);

    // Return value
    return ret;
}
