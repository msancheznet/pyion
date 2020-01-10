/* ============================================================================
 * Utility functions for the different extension modules
 *
 * Author: Marc Sanchez Net
 * Date:   8/12/2019
 * Copyright (c) 2019, California Institute of Technology ("Caltech").  
 * U.S. Government sponsorship acknowledged.
 * =========================================================================== */

#include <stdarg.h>
#include <stdio.h>

#include <bp.h>
#include <cfdp.h>
#include <ltp.h>
#include <ion.h>
#include <sdrxn.h>
#include <psm.h>

#include <Python.h>

/* ============================================================================
 * === Exception Handling and Debugging
 * ============================================================================ */

static void pyion_SetExc(PyObject *exception, const char *fmt, ...) {
    // Create pointer to list of arguments
    va_list args;
    char    err_msg[150];

    // Initialize to the first argument
    va_start(args, fmt);

    // Create the error string
    sprintf(err_msg, fmt, args);

    // Set the Python error
    PyErr_SetString(exception, err_msg);

    // Clean up arguments
    va_end(args);
}

static void pyion_vSetExc(PyObject *exception, const char *fmt, va_list args) {
    /* This function is analogous to vprintf. It allows other functions to 
       forward a variable number of arguments. */
    // Define variables
    char err_msg[150];

    // Create the error string
    sprintf(err_msg, fmt, args);

    // Set the Python error
    PyErr_SetString(exception, err_msg);
}

static void pyion_debug(const char *fmt, ...) {
    // Create pointer to list of arguments
    va_list args;
    char    err_msg[500];

    // Initialize to the first argument
    va_start(args, fmt);

    // Create the error string
    sprintf(err_msg, fmt, args);

    // Print result
    puts(err_msg);

    // Clean up arguments
    va_end(args);
}

/* ============================================================================
 * === Attach/Detach from ION
 * ============================================================================ */

static int py_ion_attach() {
    // Define variables
    int ok;

    // Attach to ION
    ok = ionAttach();

    // If success, return
    if (ok == 0) return 1;

    // Set error message and return
    pyion_SetExc(PyExc_RuntimeError, "Cannot attach to ION.");

    return 0;
}

static int py_ion_detach() {
    // Detach from ION
    ionDetach();
    return 1;
}

static int py_bp_attach() {
    // Defined variables
    int ok;

    // Attach to LTP
    ok = bp_attach();

    // If success, return
    if (ok == 0) return 1;

    // Set error message and return
    pyion_SetExc(PyExc_RuntimeError, "Cannot attach to ION's BP.");

    return 0;
}

static int py_bp_detach() {
    // Detach from ION
    bp_detach();
    return 1;
}

static int py_ltp_attach() {
    // Defined variables
    int ok;

    // Attach to LTP
    ok = ltp_attach();

    // If success, return
    if (ok == 0) return 1;

    // Set error message and return
    pyion_SetExc(PyExc_RuntimeError, "Cannot attach to ION's LTP.");

    return 0;
}

static int py_ltp_detach() {
    // Detach from ION
    ltp_detach();
    return 1;
}

static int py_cfdp_attach() {
    // Defined variables
    int ok;

    // Attach to LTP
    ok = cfdp_attach();

    // If success, return
    if (ok == 0) return 1;

    // Set error message and return
    pyion_SetExc(PyExc_RuntimeError, "Cannot attach to ION's CFDP.");

    return 0;
}

static int py_cfdp_detach() {
    // Detach from ION
    cfdp_detach();
    return 1;
}

/* ============================================================================
 * === Start/End SDR Transaction
 * ============================================================================ */

static int sdr_pybegin_xn(Sdr sdr) {
    // Define variables
    int ok = 0;

    // sdr_begin_xn can block. Therefore, release the GIL
    Py_BEGIN_ALLOW_THREADS
    ok = sdr_begin_xn(sdr);
    Py_END_ALLOW_THREADS

    // If failure to start transaction, set Exception
    if (!ok) pyion_SetExc(PyExc_RuntimeError, "[sdr_pybegin_xn] Cannot start SDR transaction.");

    // Return True if success
    return ok;
}

static int sdr_pyend_xn(Sdr sdr) {
    // End SDR transaction
    if (sdr_end_xn(sdr) < 0) {
        pyion_SetExc(PyExc_RuntimeError, "[sdr_pyend_xn] Cannot end SDR transaction.");
        return 0;
    }

    // Return True if success
    return 1;
}

static int sdr_pyexit_xn(Sdr sdr) {
    // End SDR transaction.
    sdr_exit_xn(sdr);

    // Always return success
    return 1;
}

/* ============================================================================
 * === Check ION Pointer Validity
 * ============================================================================ */

static int psm_check_addr(PsmAddress addr, PyObject *exc, const char *fmt, ...) {
    // If address is not null, return 1.
    if (addr) return 1;

    // Set the Python exception
    va_list args;
    va_start(args, fmt);
    pyion_vSetExc(exc, fmt, args);
    va_end(args);

    return 0;
}

/* ============================================================================
 * === Time-related Utility Functions
 * ============================================================================ */

static time_t _referenceTime(time_t *newValue)
{
	static time_t	reftime = 0;
	if (newValue) reftime = *newValue;
	return reftime;
}

static time_t pyion_readTimestampUTC(char *timestampStr) {
    // Define variables
    time_t refTime   = _referenceTime(NULL);
    time_t timestamp = readTimestampUTC(timestampStr, refTime);

    // Check if timestamp was correctly read
    if (timestamp == 0) {
        pyion_SetExc(PyExc_ValueError, "[pyion_readTimestampUTC] Cannot parse %s.", timestampStr);
        return 0;
    }

    return timestamp;
}