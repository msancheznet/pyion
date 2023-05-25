/* ============================================================================
 * Python C Extension to interface with ION's management functions
 * 
 * Author: Marc Sanchez Net
 * Date:   08/21/2019
 * Copyright (c) 2019, California Institute of Technology ("Caltech").  
 * U.S. Government sponsorship acknowledged.
 * =========================================================================== */

// ION Public API includes
#include <ion.h>
#include <zco.h>
#include <rfx.h>

// ION Private API includes
#include <bpP.h>
#include <ltpP.h>

// Other includes
#include <stdio.h>
#include <Python.h>
#include "_utils.c"
#include "macros.h"

/* ============================================================================
 * === _mgmt global variables
 * ============================================================================ */

// Contact is dict {orig: int, dest: int, tstart: str, tend: str, rate: float, confidence: float}
static char *py_contact_def = "{s:i, s:K, s:K, s:s, s:s, s:I, s:d}";

// Range is dict {orig: int, dest: int, tstart: str, tend: str, owlt: int}
static char *py_range_def   = "{s:K, s:K, s:s, s:s, s:I}";

// LTP span is a dict
static char *py_span_def = "{s:K, s:I, s:I, s:I, s:I, s:I, s:s, s:i, s:i, s:I, s:i}";

/* ============================================================================
 * === _mgmt module definitions
 * ============================================================================ */

 // Docstring for this module
static char module_docstring[] =
    "Extension module to interface Python and ION's management functions.";

// Docstring for the functions being wrapped
static char bp_watch_docstring[] =
    "Toggle watching BP characters.";
static char bp_endpoint_exists_docstring[] =
    "Check if a BP endpoint is defined in ION.";
static char bp_add_endpoint_docstring[] =
    "Define and add a new BP endpoint.";
static char list_regions_docstring[] =
    "List all regions in ION.";
static char list_contacts_docstring[] =
    "List all contacts in ION's contact plan.";
static char list_ranges_docstring[] =
    "List all ranges in ION's contact plan.";
static char add_contact_docstring[] =
    "Add a contacts to ION's contact plan.";
static char add_range_docstring[] =
    "Add a range in ION's contact plan.";
static char delete_contact_docstring[] =
    "Delete contact(s) in ION's contact plan.";
static char delete_range_docstring[] =
    "Delete range(s) in ION's contact plan.";
static char ltp_span_exists_docstring[] =
    "Check if an LTP span is defined in ION.";    
static char sm_task_yield_docstring[] =
    "Yield so other tasks may execute.";
static char find_span_docstring[] =
    "Find the associated span for a remote engine ID.";

// Declare the functions to wrap
static PyObject *pyion_bp_watch(PyObject *self, PyObject *args);
static PyObject *pyion_bp_endpoint_exists(PyObject *self, PyObject *args);
static PyObject *pyion_bp_add_endpoint(PyObject *self, PyObject *args);
static PyObject *pyion_list_regions(PyObject *self, PyObject *args);
static PyObject *pyion_list_contacts(PyObject *self, PyObject *args);
static PyObject *pyion_list_ranges(PyObject *self, PyObject *args);
static PyObject *pyion_add_contact(PyObject *self, PyObject *args);
static PyObject *pyion_add_range(PyObject *self, PyObject *args);
static PyObject *pyion_delete_contact(PyObject *self, PyObject *args);
static PyObject *pyion_delete_range(PyObject *self, PyObject *args);
static PyObject *pyion_ltp_span_exists(PyObject *self, PyObject *args);
static PyObject *pyion_sm_task_yield(PyObject *self, PyObject *args);
static PyObject *pyion_find_span(PyObject *self, PyObject *args);

// Define member functions of this module
static PyMethodDef module_methods[] = {
    {"bp_watch", pyion_bp_watch, METH_VARARGS, bp_watch_docstring},
    {"bp_endpoint_exists", pyion_bp_endpoint_exists, METH_VARARGS, bp_endpoint_exists_docstring},
    {"bp_add_endpoint", pyion_bp_add_endpoint, METH_VARARGS, bp_add_endpoint_docstring},
    {"list_regions", pyion_list_regions, METH_VARARGS, list_regions_docstring},
    {"list_contacts", pyion_list_contacts, METH_VARARGS, list_contacts_docstring},
    {"list_ranges", pyion_list_ranges, METH_VARARGS, list_ranges_docstring},
    {"add_contact", pyion_add_contact, METH_VARARGS, add_contact_docstring},
    {"add_range", pyion_add_range, METH_VARARGS, add_range_docstring},
    {"delete_contact", pyion_delete_contact, METH_VARARGS, delete_contact_docstring},
    {"delete_range", pyion_delete_range, METH_VARARGS, delete_range_docstring},
    {"ltp_span_exists", pyion_ltp_span_exists, METH_VARARGS, ltp_span_exists_docstring},
    {"sm_task_yield", pyion_sm_task_yield, METH_VARARGS, sm_task_yield_docstring},
    {"find_span", pyion_find_span, METH_VARARGS, find_span_docstring},
    {NULL, NULL, 0, NULL}
};

/* ============================================================================
 * === Define _mgmt as a Python module
 * ============================================================================ */

PyMODINIT_FUNC PyInit__mgmt(void) {
    // Define variables
    PyObject *module;

    // Define module configuration parameters
    static struct PyModuleDef moduledef = {
        PyModuleDef_HEAD_INIT,
        "_mgmt",
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
 * === Watch configuration functions
 * ============================================================================ */

static PyObject *pyion_bp_watch(PyObject *self, PyObject *args) {
    // Attach to ION
    if (!py_bp_attach()) return NULL;
    
    // Define variables
    int     watch;
    BpVdb	*vdb = getBpVdb();

    // Parse the input tuple. Raises error automatically if not possible
    if (!PyArg_ParseTuple(args, "i", &watch))
        return NULL;

    // If vdb is NULL, raise error
    if (!vdb) {
        pyion_SetExc(PyExc_RuntimeError, "Cannot find vdb.");
        return NULL;
    }

    // Activate watch
    if (watch)
        vdb->watching = -1;
    else
        vdb->watching = 0;

    Py_RETURN_NONE;
}

/* ============================================================================
 * === Endpoint configuration functions
 * ============================================================================ */

static PyObject *pyion_bp_endpoint_exists(PyObject *self, PyObject *args) {
    // Attach to ION
    if (!py_bp_attach()) return NULL;

    // Define variables
    Sdr		    bpSdr = getIonsdr();
    char        *eid;
    MetaEid		metaEid;
    VScheme		*vscheme;
    VEndpoint	*vpoint;
	PsmAddress	elt;

    // Parse the input tuple. Raises error automatically if not possible
    if (!PyArg_ParseTuple(args, "s", &eid)) return NULL;

    // If not sdr, raise exception
    if (bpSdr == NULL) {
        pyion_SetExc(PyExc_RuntimeError, "Cannot find SDR.");
        return NULL;
    }

    // Parse the eid
    if (parseEidString(eid, &metaEid, &vscheme, &elt) == 0) {
        pyion_SetExc(PyExc_ValueError, "Cannot parse the EID %s.", eid);
        goto error;
    }

    // Find the enpoint
    if (!sdr_pybegin_xn(bpSdr)) goto error;
    findEndpoint(NULL, &metaEid, vscheme, &vpoint, &elt);
    sdr_exit_xn(bpSdr);

    // Return value
    if (elt) goto found; else goto not_found;

found:
    restoreEidString(&metaEid);
    Py_RETURN_TRUE;
not_found:
    restoreEidString(&metaEid);
    Py_RETURN_FALSE;
error:
    restoreEidString(&metaEid);
    return NULL;
}

static PyObject *pyion_bp_add_endpoint(PyObject *self, PyObject *args) {
    // Attach to ION
    if (!py_bp_attach()) return NULL;

    // Define variables
    char *eid = NULL;
    int discard, ok;
    BpRecvRule	rule;

    // Parse the input tuple. Raises error automatically if not possible
    if (!PyArg_ParseTuple(args, "si", &eid, &discard))
        return NULL;

    // Set the receive rule
    if (discard == 1)
        rule = DiscardBundle;
    else
        rule = EnqueueBundle;
    
    // Add endpoint
    ok = addEndpoint(eid, rule, NULL);

    // Raise exception if error
    if (ok == 0) {
        pyion_SetExc(PyExc_RuntimeError, "Cannot open endpoint %s.", eid);
        return NULL;
    }
    
    Py_RETURN_NONE;
}

/* ============================================================================
 * === Region functions
 * ============================================================================ */

static PyObject *pyion_list_regions(PyObject *self, PyObject *args) {
    // Attach to ION
    if (!py_ion_attach()) return NULL;

    // Define variables
    char err_msg[150];
    Sdr	sdr = getIonsdr();
    Object	iondbObj;
	IonDB	iondb;
    int	i;
    PyObject *regions = PyList_New(0);

    // Initialize variables
    iondbObj = getIonDbObject();
    if (!iondbObj) {
        pyion_SetExc(PyExc_RuntimeError, "Cannot find ION database.");
        return NULL;
    }

    // Get data from the SDR
    Py_BEGIN_ALLOW_THREADS
    if (!sdr_begin_xn(sdr)) {
        pyion_SetExc(PyExc_RuntimeError, "Cannot start SDR transaction.");
        return NULL;
    }
    sdr_read(sdr, (char *) &iondb, iondbObj, sizeof(IonDB));
    sdr_exit_xn(sdr);
    Py_END_ALLOW_THREADS

    // Build output list
    for (i = 0; i < 2; i++) {
        PyObject *item = Py_BuildValue("i", (int)iondb.regions[i].regionNbr);
        PyList_Append(regions, item);
	}

    // Return list
    return regions;
}

/* ============================================================================
 * === Contact plan functions
 * ============================================================================ */

static PyObject *pyion_list_contacts(PyObject *self, PyObject *args) {
    // Attach to ION
    if (!py_ion_attach()) return NULL;

    // Define variables
    PsmAddress	    elt, addr;
    IonCXref	    *contact;
    char		    fromTimeBuffer[TIMESTAMPBUFSZ];
    char		    toTimeBuffer[TIMESTAMPBUFSZ];

    // Get ION SDR and PSM
    Sdr		        sdr = getIonsdr();
    PsmPartition	ionwm = getIonwm();
    IonVdb		    *vdb = getIonVdb();

    // Initialize empty Python list
    PyObject* py_contacts = PyList_New(0);

    // Start SDR transaction
    if (!sdr_pybegin_xn(sdr)) return NULL;

    // Iterate over all contacts
    for (elt = sm_rbt_first(ionwm, vdb->contactIndex); elt; elt = sm_rbt_next(ionwm, elt)) {
        // Get the address where data is located
        addr = sm_rbt_data(ionwm, elt);
        
        // Check address validity
        if (!psm_check_addr(addr, PyExc_RuntimeError, "[pyion_list_contacts] Invalid pointer.")) return NULL;

        // Get contact info
        contact = (IonCXref *) psp(getIonwm(), addr);
        writeTimestampUTC(contact->fromTime, fromTimeBuffer);
        writeTimestampUTC(contact->toTime, toTimeBuffer);

        // Save this contact information
        PyList_Append(py_contacts, Py_BuildValue(py_contact_def, "region_nbr", contact->regionNbr,
                                                "orig", contact->fromNode, "dest",
                                                contact->toNode, "tstart", fromTimeBuffer, "tend",
                                                toTimeBuffer, "rate", 8*contact->xmitRate, "confidence",
                                                contact->confidence));
    }

    // Exit SDR transaction
    sdr_pyexit_xn(sdr);

    return py_contacts;
}

static PyObject *pyion_list_ranges(PyObject *self, PyObject *args) {
    // Attach to ION
    if (!py_ion_attach()) return NULL;

    // Define variables
    PsmAddress	    elt, addr;
    IonRXref	    *range;
    char		    fromTimeBuffer[TIMESTAMPBUFSZ];
    char		    toTimeBuffer[TIMESTAMPBUFSZ];

    // Get ION SDR and PSM
    Sdr		        sdr = getIonsdr();
    PsmPartition	ionwm = getIonwm();
    IonVdb		    *vdb = getIonVdb();

    // Initialize empty Python list
    PyObject* py_ranges = PyList_New(0);

    // Start SDR transaction
    if (!sdr_pybegin_xn(sdr)) return NULL;

    // Iterate over all contacts
    for (elt = sm_rbt_first(ionwm, vdb->rangeIndex); elt; elt = sm_rbt_next(ionwm, elt)) {
        // Get the address where data is located
        addr = sm_rbt_data(ionwm, elt);
        
        // Check address validity
        if (!psm_check_addr(addr, PyExc_RuntimeError, "[pyion_list_ranges] Invalid pointer.")) return NULL;

        // Get contact info
        range = (IonRXref *) psp(getIonwm(), addr);
        writeTimestampUTC(range->fromTime, fromTimeBuffer);
        writeTimestampUTC(range->toTime, toTimeBuffer);

        // Save this contact information
        PyList_Append(py_ranges, Py_BuildValue(py_range_def, "orig", range->fromNode, "dest",
                                               range->toNode, "tstart", fromTimeBuffer, "tend",
                                               toTimeBuffer, "owlt", range->owlt));
    }

    // Exit SDR transaction
    sdr_pyexit_xn(sdr);

    return py_ranges;
}

static PyObject *pyion_add_contact(PyObject *self, PyObject *args) {
    // Attach to ION
    if (!py_ion_attach()) return NULL;

    // Define variables
    PsmAddress	xaddr;
    uint32_t regionIdx;
    uvast fromNode, toNode;
    time_t fromTime, toTime;
    char *fromTimeStr;
    char *toTimeStr;
    unsigned int xmitRate;
    float confidence;
    int announce;
    int ok;

    // Parse the input tuple. Raises error automatically if not possible
    if (!PyArg_ParseTuple(args, "iKKssIfi", (int*) &regionIdx, (unsigned long long *)&fromNode, 
                        (unsigned long long *)&toNode, &fromTimeStr, &toTimeStr, &xmitRate, &confidence,
                        &announce))
        return NULL;

    // Parse the timestamps
    fromTime = pyion_readTimestampUTC(fromTimeStr);
    toTime   = pyion_readTimestampUTC(toTimeStr);

    // Check validity of parsed timestamps
    if (fromTime == 0) {
        pyion_SetExc(PyExc_ValueError, "Cannot parse tstart=%s.", fromTimeStr);
        return NULL;
    }
    if (toTime == 0) {
        pyion_SetExc(PyExc_ValueError, "Cannot parse tend=%s.", toTimeStr);
        return NULL;
    }

    // Insert a contact
    ok = rfx_insert_contact(regionIdx, fromTime, toTime, fromNode, toNode, xmitRate, confidence, &xaddr, announce);
    if (ok < 0) {
        pyion_SetExc(PyExc_RuntimeError, "Error in rfx_insert_contact.");
        return NULL;
    }

    Py_RETURN_NONE;
}

static PyObject *pyion_add_range(PyObject *self, PyObject *args) {
    // Attach to ION
    if (!py_ion_attach()) return NULL;

    // Define variables
    PsmAddress	xaddr;
    uvast fromNode, toNode;
    time_t fromTime, toTime;
    char *fromTimeStr;
    char *toTimeStr;
    unsigned int owlt;
    int announce;

    // Parse the input tuple. Raises error automatically if not possible
    if (!PyArg_ParseTuple(args, "KKssIi", (unsigned long long *)&fromNode, (unsigned long long *)&toNode,
                        &fromTimeStr, &toTimeStr, &owlt, &announce))
        return NULL;

    // Parse the timestamps
    fromTime = pyion_readTimestampUTC(fromTimeStr);
    toTime   = pyion_readTimestampUTC(toTimeStr);

    // Check validity of parsed timestamps
    if (!fromTime) {
        pyion_SetExc(PyExc_ValueError, "Cannot parse tstart=%s.", fromTimeStr);
        return NULL;
    }
    if (!toTime) {
        pyion_SetExc(PyExc_ValueError, "Cannot parse tend=%s.", toTimeStr);
        return NULL;
    }

    // Insert a range
    oK(rfx_insert_range(fromTime, toTime, fromNode, toNode, owlt, &xaddr, announce));

    Py_RETURN_NONE;
}

static PyObject *pyion_delete_contact(PyObject *self, PyObject *args) {
    // Attach to ION
    if (!py_ion_attach()) return NULL;

    // Define variables
    uint32_t regionNbr;
    char *fromTimeStr = NULL;
    time_t fromTime_val;
    uvast fromNode, toNode;
    time_t* fromTime = NULL;
    int announce;

    // Parse the input tuple. Raises error automatically if not possible
    if (!PyArg_ParseTuple(args, "iKKzi", (int*)&regionNbr, (unsigned long long *)&fromNode,  
                          (unsigned long long *)&toNode, &fromTimeStr, &announce))
        return NULL;

    // If fromTime is None, set to 0
    if (fromTimeStr) {
        fromTime_val = pyion_readTimestampUTC(fromTimeStr);
        fromTime = &fromTime_val;
        
        // Raise exception if parsing timestamp failed
        if (!fromTime) {
            pyion_SetExc(PyExc_ValueError, "Cannot parse tstart=%s", fromTimeStr);
            return NULL; 
        }
    }

    // Delete the contact(s)
    oK(rfx_remove_contact(regionNbr, fromTime, fromNode, toNode, announce));

    Py_RETURN_NONE;
}

static PyObject *pyion_delete_range(PyObject *self, PyObject *args) {
    // Attach to ION
    if (!py_ion_attach()) return NULL;

    // Define variables
    char *fromTimeStr = NULL;
    uvast fromNode, toNode;
    time_t fromTime;
    int announce;

    // Parse the input tuple. Raises error automatically if not possible
    if (!PyArg_ParseTuple(args, "KKzi", (unsigned long long *)&fromNode, (unsigned long long *)&toNode, 
                        &fromTimeStr, &announce))
        return NULL;

    // If fromTime is None, set to 0
    if (!fromTimeStr) {
        fromTime = 0;
    } else {
        fromTime = pyion_readTimestampUTC(fromTimeStr);
        
        // Raise exception if parsing timestamp failed
        if (!fromTime) {
            pyion_SetExc(PyExc_ValueError, "Cannot parse tstart=%s", fromTimeStr);
            return NULL;
        }
    }

    // Delete the contact(s)
    oK(rfx_remove_range(&fromTime, fromNode, toNode, announce));

    Py_RETURN_NONE;
}

/* ============================================================================
 * === LTP management functions
 * ============================================================================ */

// This is not an exposed Python wrapper
static int _find_span(uvast engineNbr, PsmAddress *elt) {
    // Define variables
    Sdr      sdr = getIonsdr();
    LtpVspan *vspan;

    // If not sdr, set exception and return
    if (sdr == NULL) {
        pyion_SetExc(PyExc_RuntimeError, "Cannot find SDR.");
        return 0;
    }

    // Find the enpoint
    if (!sdr_pybegin_xn(sdr)) return 0; 
    findSpan(engineNbr, &vspan, elt);
    sdr_exit_xn(sdr);

    return 1;
}

static PyObject *pyion_ltp_span_exists(PyObject *self, PyObject *args) {
    // Attach to ION
    if (!py_ltp_attach()) return NULL;

    // Define variables
    PsmAddress elt;
    unsigned long long nbr;
    
    // Parse the input tuple. Raises error automatically if not possible
    if (!PyArg_ParseTuple(args, "K", &nbr)) goto error;

    // Find the span
    if (!_find_span((uvast)nbr, &elt)) goto error;

    // See if found
    if (elt) goto found; else goto not_found;

error:
    return NULL;
found:
    Py_RETURN_TRUE;
not_found:
    Py_RETURN_FALSE;
}

static PyObject *pyion_find_span(PyObject *self, PyObject *args) {
    unsigned long long remoteEngineId;

    if (!PyArg_ParseTuple(args, "K", &remoteEngineId)) {
        return NULL;
    }

    LtpVspan *vspan;
    PsmAddress vspanElt; // Is an unsigned long long (U64)

    Sdr sdr = getIonsdr();
    if (!sdr_begin_xn(sdr))
    {
        PyErr_SetString(PyExc_RuntimeError, "Error starting SDR transaction.");
        return NULL;
    }

    findSpan(remoteEngineId, &vspan, &vspanElt);
    if (vspanElt == 0)
    {
        sdr_exit_xn(sdr);
        Py_RETURN_NONE;
    }

    sdr_exit_xn(sdr);

    // TODO hacky but trying to return vspan address
    uintptr_t vspan_addr = (uintptr_t)vspan;
    return PyLong_FromUnsignedLongLong((unsigned long long)vspan_addr);
}

static PyObject *pyion_sm_task_yield(PyObject *self, PyObject *args) {
    sm_TaskYield();
    Py_RETURN_NONE;
}
