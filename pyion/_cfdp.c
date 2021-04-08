/* ============================================================================
 * Experimental Python C Extension for CFDP
 * 
 * Author: Marc Sanchez Net
 * Date:   04/26/2019
 * Copyright (c) 2019, California Institute of Technology ("Caltech").  
 * U.S. Government sponsorship acknowledged.
 * =========================================================================== */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <Python.h>

#include <cfdp.h>
#include <bputa.h>
#include "base_cfdp.h"

/* ============================================================================
 * === _cfdp module definitions
 * ============================================================================ */

// Docstring for this module
static char module_docstring[] =
    "Extension module to interface Python and CFDP in ION.";

// Docstring for the functions being wrapped
static char cfdp_attach_docstring[] =
    "Attach to CFDP agent.";
static char cfdp_detach_docstring[] =
    "Detach from CFDP agent.";
static char cfdp_open_docstring[] =
    "Open and CFDP Entity object.";
static char cfdp_close_docstring[] =
    "Close a CFDP Entity object.";
static char cfdp_send_docstring[] =
    "Send a file to another host using CFDP.";
static char cfdp_request_docstring[] =
    "Request a file from another host using CFDP.";
static char cfdp_cancel_docstring[] =
    "Cancel a CFDP transaction.";
static char cfdp_suspend_docstring[] =
    "Suspend a CFDP transaction.";
static char cfdp_resume_docstring[] =
    "Resume a CFDP transaction.";
static char cfdp_report_docstring[] =
    "Report a CFDP transaction.";
static char cfdp_add_usr_msg_docstring[] =
    "Add a user message to the next CFDP transaction.";
static char cfdp_add_fs_req_docstring[] =
    "Add a user message to the next CFDP transaction.";    
static char cfdp_next_evs_docstring[] =
    "Handle CFDP events.";    
static char cfdp_interrupt_evs_docstring[] =
    "Handle CFDP events.";    

// Declare the functions to wrap
static PyObject *pyion_cfdp_attach(PyObject *self, PyObject *args);
static PyObject *pyion_cfdp_detach(PyObject *self, PyObject *args);
static PyObject *pyion_cfdp_open(PyObject *self, PyObject *args);
static PyObject *pyion_cfdp_close(PyObject *self, PyObject *args);
static PyObject *pyion_cfdp_send(PyObject *self, PyObject *args);
static PyObject *pyion_cfdp_request(PyObject *self, PyObject *args);
static PyObject *pyion_cfdp_cancel(PyObject *self, PyObject *args);
static PyObject *pyion_cfdp_suspend(PyObject *self, PyObject *args);
static PyObject *pyion_cfdp_resume(PyObject *self, PyObject *args);
static PyObject *pyion_cfdp_report(PyObject *self, PyObject *args);
static PyObject *pyion_cfdp_add_usr_msg(PyObject *self, PyObject *args);
static PyObject *pyion_cfdp_add_fs_req(PyObject *self, PyObject *args);
static PyObject *pyion_cfdp_next_events(PyObject *self, PyObject *args);
static PyObject *pyion_cfdp_interrupt_events(PyObject *self, PyObject *args);

// Define member functions of this module
static PyMethodDef module_methods[] = {
    {"cfdp_attach", pyion_cfdp_attach, METH_VARARGS, cfdp_attach_docstring},
    {"cfdp_detach", pyion_cfdp_detach, METH_VARARGS, cfdp_detach_docstring},
    {"cfdp_open", pyion_cfdp_open, METH_VARARGS, cfdp_open_docstring},
    {"cfdp_close", pyion_cfdp_close, METH_VARARGS, cfdp_close_docstring},
    {"cfdp_send", pyion_cfdp_send, METH_VARARGS, cfdp_send_docstring},
    {"cfdp_request", pyion_cfdp_request, METH_VARARGS, cfdp_request_docstring},
    {"cfdp_cancel", pyion_cfdp_cancel, METH_VARARGS, cfdp_cancel_docstring},
    {"cfdp_suspend", pyion_cfdp_suspend, METH_VARARGS, cfdp_suspend_docstring},
    {"cfdp_resume", pyion_cfdp_resume, METH_VARARGS, cfdp_resume_docstring},
    {"cfdp_report", pyion_cfdp_report, METH_VARARGS, cfdp_report_docstring},
    {"cfdp_add_usr_msg", pyion_cfdp_add_usr_msg, METH_VARARGS, cfdp_add_usr_msg_docstring},
    {"cfdp_add_filestore_request", pyion_cfdp_add_fs_req, METH_VARARGS, cfdp_add_fs_req_docstring},
    {"cfdp_next_event", pyion_cfdp_next_events, METH_VARARGS, cfdp_next_evs_docstring},
    {"cfdp_interrupt_events", pyion_cfdp_interrupt_events, METH_VARARGS, cfdp_interrupt_evs_docstring},
    {NULL, NULL, 0, NULL}
};

/* ============================================================================
 * === Define _cfdp as a Python module
 * ============================================================================ */

PyMODINIT_FUNC PyInit__cfdp(void) {
    // Initialize variables
    PyObject *module;

    // Define module configuration parameters
    static struct PyModuleDef moduledef = {
        PyModuleDef_HEAD_INIT,
        "_cfdp",
        module_docstring,
        -1,
        module_methods,
        NULL,
        NULL,
        NULL,
        NULL};

    // Create the module
    module = PyModule_Create(&moduledef);

    // Add CFDP Event Types
    PyModule_AddIntConstant(module, "CfdpNoEvent", CfdpNoEvent);
    PyModule_AddIntConstant(module, "CfdpTransactionInd", CfdpTransactionInd);
    PyModule_AddIntConstant(module, "CfdpEofSentInd", CfdpEofSentInd);
    PyModule_AddIntConstant(module, "CfdpTransactionFinishedInd", CfdpTransactionFinishedInd);
    PyModule_AddIntConstant(module, "CfdpMetadataRecvInd", CfdpMetadataRecvInd);
    PyModule_AddIntConstant(module, "CfdpFileSegmentRecvInd", CfdpFileSegmentRecvInd);
    PyModule_AddIntConstant(module, "CfdpEofRecvInd", CfdpEofRecvInd);
    PyModule_AddIntConstant(module, "CfdpSuspendedInd", CfdpSuspendedInd);
    PyModule_AddIntConstant(module, "CfdpResumedInd", CfdpResumedInd);
    PyModule_AddIntConstant(module, "CfdpReportInd", CfdpReportInd);
    PyModule_AddIntConstant(module, "CfdpFaultInd", CfdpFaultInd);
    PyModule_AddIntConstant(module, "CfdpAbandonedInd", CfdpAbandonedInd);
    
    // Add CFDP Condition
    PyModule_AddIntConstant(module, "CfdpNoError", CfdpNoError);
    PyModule_AddIntConstant(module, "CfdpAckLimitReached", CfdpAckLimitReached);
    PyModule_AddIntConstant(module, "CfdpKeepaliveLimitReached", CfdpKeepaliveLimitReached);
    PyModule_AddIntConstant(module, "CfdpInvalidTransmissionMode", CfdpInvalidTransmissionMode);
    PyModule_AddIntConstant(module, "CfdpFilestoreRejection", CfdpFilestoreRejection);
    PyModule_AddIntConstant(module, "CfdpChecksumFailure", CfdpChecksumFailure);
    PyModule_AddIntConstant(module, "CfdpFileSizeError", CfdpFileSizeError);
    PyModule_AddIntConstant(module, "CfdpNakLimitReached", CfdpNakLimitReached);
    PyModule_AddIntConstant(module, "CfdpInactivityDetected", CfdpInactivityDetected);
    PyModule_AddIntConstant(module, "CfdpInvalidFileStructure", CfdpInvalidFileStructure);
    PyModule_AddIntConstant(module, "CfdpCheckLimitReached", CfdpCheckLimitReached);
    PyModule_AddIntConstant(module, "CfdpSuspendRequested", CfdpSuspendRequested);
    PyModule_AddIntConstant(module, "CfdpCancelRequested", CfdpCancelRequested);

    // Add CFDP File Status
    PyModule_AddIntConstant(module, "CfdpFileDiscarded", CfdpFileDiscarded);
    PyModule_AddIntConstant(module, "CfdpFileRejected", CfdpFileRejected);
    PyModule_AddIntConstant(module, "CfdpFileRetained", CfdpFileRetained);
    PyModule_AddIntConstant(module, "CfdpFileStatusUnreported", CfdpFileStatusUnreported);

    // Add CFDP file delivery code
    PyModule_AddIntConstant(module, "CfdpDataComplete", CfdpDataComplete);
    PyModule_AddIntConstant(module, "CfdpDataIncomplete", CfdpDataIncomplete);

    // Add CFDP Actions
    PyModule_AddIntConstant(module, "CfdpCreateFile", CfdpCreateFile);
    PyModule_AddIntConstant(module, "CfdpDeleteFile", CfdpDeleteFile);
    PyModule_AddIntConstant(module, "CfdpRenameFile", CfdpRenameFile);
    PyModule_AddIntConstant(module, "CfdpAppendFile", CfdpAppendFile);
    PyModule_AddIntConstant(module, "CfdpReplaceFile", CfdpReplaceFile);
    PyModule_AddIntConstant(module, "CfdpCreateDirectory", CfdpCreateDirectory);
    PyModule_AddIntConstant(module, "CfdpRemoveDirectory", CfdpRemoveDirectory);
    PyModule_AddIntConstant(module, "CfdpDenyFile", CfdpDenyFile);
    PyModule_AddIntConstant(module, "CfdpDenyDirectory", CfdpDenyDirectory);

    // If module creation failed, return error
    if (!module) return NULL;

    return module;
}

/* ============================================================================
 * === Define structures for this module
 * ============================================================================ */


/* ============================================================================
 * === Define global variables
 * ============================================================================ */

/* ============================================================================
 * === Attach/Detach Functions
 * ============================================================================ */

static PyObject *pyion_cfdp_attach(PyObject *self, PyObject *args) {
    // Define variables
    char err_msg[150];

    // Try to attach to BP agent
    int result;

    Py_BEGIN_ALLOW_THREADS
    result = base_cfdp_attach();
    Py_END_ALLOW_THREADS

    if (result < 0) {
        sprintf(err_msg, "Cannot attach to CFDP engine. Is ION running on this host? If so, is CFDP being used?");
        PyErr_SetString(PyExc_SystemError, err_msg);
        return NULL;
    }

    // Return True to indicate success
    Py_RETURN_NONE;
}

static PyObject *pyion_cfdp_detach(PyObject *self, PyObject *args) {
    // Detach from BP agent
    base_cfdp_detach();

    // Return True to indicate success
    Py_RETURN_NONE;
}

/* ============================================================================
 * === Open/Close Entity Functions
 * ============================================================================ */

static PyObject *pyion_cfdp_open(PyObject *self, PyObject *args) {
    // Define variables
    char err_msg[150];
    uvast entityId;
    int criticality;

    // Allocate memory for CFDP state variable
    CfdpReqParms *params = (CfdpReqParms*)malloc(sizeof(CfdpReqParms));
    if (params == NULL) {
        sprintf(err_msg, "Cannot malloc for CFDP parameters");
        PyErr_SetString(PyExc_RuntimeError, err_msg);
        return NULL;
    }

    // Set memory contents to zeros
    memset((char *)params, 0, sizeof(CfdpReqParms));

    // Parse the input tuple. Raises error automatically if not possible
    if (!PyArg_ParseTuple(args, "Kiiiii", \
    (unsigned long long *)&entityId, 
    &(params->utParms.lifespan), 
    &(params->utParms.classOfService),
    &(params->utParms.ancillaryData.ordinal), 
    &(params->utParms.srrFlags), 
    &(criticality)))
        return NULL;

    // Initialize variables
    //Py_BEGIN_ALLOW_THREADS;
    base_cfdp_open(params, entityId, criticality);
    //Py_END_ALLOW_THREADS;
    // Return the memory address of the SAP for this endpoint as an unsined long
    PyObject *ret = Py_BuildValue("k", params);
    return ret;
}

static PyObject *pyion_cfdp_close(PyObject *self, PyObject *args) {
    // Define variables
    CfdpReqParms *params;

    // Parse the input tuple. Raises error automatically if not possible
    if (!PyArg_ParseTuple(args, "k", (unsigned long *)&params))
        return NULL;

    // Free the memory
    free(params);

    // Return True to indicate success
    Py_RETURN_NONE;
}

/* ============================================================================
 * === Add user messages and filestore requests
 * ============================================================================ */

static PyObject *pyion_cfdp_add_usr_msg(PyObject *self, PyObject *args) {
    // Define variables
    CfdpReqParms *params;
    char *usrMsg;

    // Parse the input tuple. Raises error automatically if not possible
    if (!PyArg_ParseTuple(args, "kz", (unsigned long *)&params, &usrMsg))
        return NULL;

    // If no user message, return
    if (usrMsg == NULL)
        Py_RETURN_NONE;

    // Create user message list if necessary
    if (params->msgsToUser == 0)
        params->msgsToUser = base_cfdp_create_usrmsg_list();

    // Add user message
    oK(base_cfdp_add_usrmsg(params->msgsToUser, (unsigned char *)usrMsg));

    // Return True to indicate success
    Py_RETURN_NONE;
}

static PyObject *pyion_cfdp_add_fs_req(PyObject *self, PyObject *args) {
    // Define variables
    CfdpReqParms *params;
    CfdpAction	action;
    char		*firstPathName;
	char		*secondPathName;

    // Parse the input tuple. Raises error automatically if not possible
    if (!PyArg_ParseTuple(args, "kisz", (unsigned long *)&params, (int *)&action,
                          &firstPathName, &secondPathName))
        return NULL;

    // Create a file request list if necessary
    if (params->fsRequests == 0)
        params->fsRequests = cfdp_create_fsreq_list();

    // Add file request
    oK(cfdp_add_fsreq(params->fsRequests, action, firstPathName, secondPathName));

    // Return True to indicate success
    Py_RETURN_NONE;
}

/* ============================================================================
 * === Send/Request Functions (and helpers)
 * ============================================================================ */

static int	noteSegmentTime(uvast fileOffset, unsigned int recordOffset,
			unsigned int length, int sourceFileFd, char *buffer) {
	writeTimestampLocal(getCtime(), buffer);
	return strlen(buffer) + 1;
}

static void setParams(CfdpReqParms *params, char *sourceFile, char *destFile, 
                      int segMetadata, int closureLat, long int mode) {
    // Fill in basic parameters
    params->sourceFileName = sourceFile;
    params->destFileName   = destFile;
    params->segMetadataFn = (segMetadata==0) ? NULL : noteSegmentTime;
    params->closureLatency = closureLat;

    // mode = 1: Select unreliable CFDP mode
    if (mode & 0x01) {
        params->utParms.ancillaryData.flags |= BP_BEST_EFFORT;
        return;
    }

    // mode = 2: Select native BP reliability
    if (mode & 0x02) {
        params->utParms.custodySwitch = SourceCustodyRequired;
        return;
    }

    // Mode = 3: Select CL reliability
    params->utParms.ancillaryData.flags &= (~BP_BEST_EFFORT);
    params->utParms.custodySwitch = NoCustodyRequested;
}

static PyObject *pyion_cfdp_send(PyObject *self, PyObject *args) {
    // Define variables
    char err_msg[150];
    CfdpReqParms *params;
    char *sourceFile;
    char *destFile;
    int closureLat, segMetadata;
    long int mode;

    // Parse the input tuple. Raises error automatically if not possible
    if (!PyArg_ParseTuple(args, "ksziil", (unsigned long *)&params, &sourceFile, 
                          &destFile, &closureLat, &segMetadata, &mode))
        return NULL;

    // Store parameters
    setParams(params, sourceFile, destFile, segMetadata, closureLat, mode);

    // Trigger the CFDP put
    if (cfdp_put(&(params->destinationEntityNbr), sizeof(BpUtParms), 
                 (unsigned char *) &(params->utParms), params->sourceFileName,
				 params->destFileName, NULL, params->segMetadataFn,
				 NULL, 0, NULL, params->closureLatency, params->msgsToUser,
				 params->fsRequests, &(params->transactionId)) < 0){
        sprintf(err_msg, "Cannot do cfdp_put operation, check ion.log.");                     
        PyErr_SetString(PyExc_RuntimeError, err_msg);
        return NULL;
    }

    // Reset parameters
    params->msgsToUser = 0;
    params->fsRequests = 0;

    // Return True to indicate success
    Py_RETURN_NONE;
}

static PyObject *pyion_cfdp_request(PyObject *self, PyObject *args) {
    // Define variables
    char err_msg[150];
    CfdpReqParms *params;
    CfdpProxyTask task;
    char *sourceFile;
    char *destFile;
    int closureLat, segMetadata;
    long int mode;

    // Parse the input tuple. Raises error automatically if not possible
    if (!PyArg_ParseTuple(args, "ksziil", (unsigned long *)&params, &sourceFile, 
                          &destFile, &closureLat, &segMetadata, &mode))
        return NULL;

    // Store parameters
    setParams(params, sourceFile, destFile, segMetadata, closureLat, mode);

    // Create CFDP Task structure
    task.sourceFileName = params->sourceFileName;
    task.destFileName = params->destFileName;
    task.messagesToUser = params->msgsToUser;
    task.filestoreRequests = params->fsRequests;
    task.faultHandlers = NULL;
    task.unacknowledged = 1;
    task.flowLabelLength = 0;
    task.flowLabel = NULL;
    task.recordBoundsRespected = 0;
    task.closureRequested = !(params->closureLatency == 0);

    // Tigger CFDP get command
    if (cfdp_get(&(params->destinationEntityNbr), sizeof(BpUtParms),
					(unsigned char *) &(params->utParms), NULL, NULL, NULL, 
                    NULL, 0, NULL, 0, 0, 0, &task, &(params->transactionId)) < 0) {
        sprintf(err_msg, "Cannot do cfdp_get operation, check ion.log.");                     
        PyErr_SetString(PyExc_RuntimeError, err_msg);
        return NULL;
    }

    // Reset parameters
    params->msgsToUser = 0;
    params->fsRequests = 0;

    // Return True to indicate success
    Py_RETURN_NONE;
}

/* ============================================================================
 * === Cancel/Suspend/Resume/Report Functions
 * ============================================================================ */

static PyObject *pyion_cfdp_cancel(PyObject *self, PyObject *args) {
    // Define variables
    char err_msg[150];
    CfdpReqParms *params;

    // Parse the input tuple. Raises error automatically if not possible
    if (!PyArg_ParseTuple(args, "k", (unsigned long *)&params))
        return NULL;

    // Trigger CFDP cancel
    if (cfdp_cancel(&(params->transactionId)) < 0) {
        sprintf(err_msg, "Cannot do cfdp_cancel operation, check ion.log.");                     
        PyErr_SetString(PyExc_RuntimeError, err_msg);
        return NULL;
    }

    // Return True to indicate success
    Py_RETURN_NONE;
}

static PyObject *pyion_cfdp_suspend(PyObject *self, PyObject *args) {
    // Define variables
    char err_msg[150];
    CfdpReqParms *params;

    // Parse the input tuple. Raises error automatically if not possible
    if (!PyArg_ParseTuple(args, "k", (unsigned long *)&params))
        return NULL;

    // Trigger CFDP suspend
    if (cfdp_suspend(&(params->transactionId)) < 0) {
        sprintf(err_msg, "Cannot do cfdp_suspend operation, check ion.log.");                     
        PyErr_SetString(PyExc_RuntimeError, err_msg);
        return NULL;
    }

    // Return True to indicate success
    Py_RETURN_NONE;
}

static PyObject *pyion_cfdp_resume(PyObject *self, PyObject *args) {
    // Define variables
    char err_msg[150];
    CfdpReqParms *params;

    // Parse the input tuple. Raises error automatically if not possible
    if (!PyArg_ParseTuple(args, "k", (unsigned long *)&params))
        return NULL;

    // Trigger CFDP resume
    if (cfdp_resume(&(params->transactionId)) < 0) {
        sprintf(err_msg, "Cannot do cfdp_resume operation, check ion.log.");                     
        PyErr_SetString(PyExc_RuntimeError, err_msg);
        return NULL;
    }

    // Return True to indicate success
    Py_RETURN_NONE;
}

static PyObject *pyion_cfdp_report(PyObject *self, PyObject *args) {
    // Define variables
    char err_msg[150];
    CfdpReqParms *params;

    // Parse the input tuple. Raises error automatically if not possible
    if (!PyArg_ParseTuple(args, "k", (unsigned long *)&params))
        return NULL;

    // Trigger CFDP report
    if (cfdp_report(&(params->transactionId)) < 0) {
        sprintf(err_msg, "Cannot do cfdp_report operation, check ion.log.");                     
        PyErr_SetString(PyExc_RuntimeError, err_msg);
        return NULL;
    }

    // Return None to indicate success
    Py_RETURN_NONE;
}

/* ============================================================================
 * === Handling of CFDP Events (see CCSDS CDFP, section 3.5.6 onwards)
 * ============================================================================ */

static PyObject *pyion_cfdp_next_events(PyObject *self, PyObject *args) {
    // Define variables for cfdp_get_event
    CfdpEventType type;
    time_t time;
    int reqNbr;
    CfdpTransactionId transactionId;
	char sourceFileNameBuf[256];
	char destFileNameBuf[256];
    uvast			fileSize;
	MetadataList		messagesToUser;
	uvast			offset;
	unsigned int		length;
	unsigned int		recordBoundsRespected;
	CfdpContinuationState	continuationState;
	unsigned int		segMetadataLength;
	char			segMetadata[63];
	CfdpCondition		condition;
	uvast			progress;
	CfdpFileStatus		fileStatus;
	CfdpDeliveryCode	deliveryCode;
	CfdpTransactionId	originatingTransactionId;
	char			statusReportBuf[256];
	MetadataList		filestoreResponses;
    
    // Define other variables
    char err_msg[150];
    int rx_ret;
    uvast transaction_id, source_entity_nbr;

    // Receive the next CFDP event. This is a blocking call
    Py_BEGIN_ALLOW_THREADS                                // Release the GIL
    rx_ret = cfdp_get_event(&type, &time, &reqNbr, &transactionId,
				sourceFileNameBuf, destFileNameBuf,
				&fileSize, &messagesToUser, &offset, &length,
				&recordBoundsRespected, &continuationState,
				&segMetadataLength, segMetadata,
				&condition, &progress, &fileStatus,
				&deliveryCode, &originatingTransactionId,
				statusReportBuf, &filestoreResponses);
    Py_END_ALLOW_THREADS                                  // Acquire the GIL

    // If reception of event failed, return
    if (rx_ret < 0) {
        sprintf(err_msg, "Failed while getting CFDP event, check ion.log.");                     
        PyErr_SetString(PyExc_RuntimeError, err_msg);
        return NULL;
    }

    // If no event, just return
    if (type == CfdpNoEvent) {
        return Py_BuildValue("(i, z)", (int)CfdpNoEvent, NULL);
    }

    // Handle CfdpTransactionInd
    if (type == CfdpTransactionInd) {
        cfdp_decompress_number(&transaction_id, &(transactionId.transactionNbr));
        return Py_BuildValue("(i, {s:K})", (int)CfdpTransactionInd, 
                                           "transaction_id", (unsigned long long)transaction_id);
    }

    // Handle CfdpEofSentInd
    if (type == CfdpEofSentInd) {
        cfdp_decompress_number(&transaction_id, &(transactionId.transactionNbr));
        return Py_BuildValue("(i, {s:K})", (int)CfdpEofSentInd, 
                                           "transaction_id", (unsigned long long)transaction_id);
    }

    // Handle CfdpEofRecvInd
    if (type == CfdpEofRecvInd) {
        cfdp_decompress_number(&transaction_id, &(transactionId.transactionNbr));
        return Py_BuildValue("(i, {s:K})", (int)CfdpEofRecvInd, 
                                           "transaction_id", (unsigned long long)transaction_id);
    }

    // Handle CfdpSuspendedInd
    if (type == CfdpSuspendedInd) {
        return Py_BuildValue("(i, {s:i})", (int)CfdpSuspendedInd, "condition", (int)condition);
    }

    // Handle CfdpResumedInd
    if (type == CfdpResumedInd) {
        return Py_BuildValue("(i, {s:K})", (int)CfdpResumedInd, "progress", (unsigned long long)progress);
    }

    // Handle CfdpReportInd
    if (type == CfdpReportInd) {
        cfdp_decompress_number(&transaction_id, &(transactionId.transactionNbr));
        return Py_BuildValue("(i, {s:K, s:i})", (int)CfdpReportInd, "transaction_id",
                                                (unsigned long long)transaction_id,
                                                "status", (int)fileStatus);

    }

    // Handle CfdpFaultInd
    if (type == CfdpFaultInd) {
        cfdp_decompress_number(&transaction_id, &(transactionId.transactionNbr));
        return Py_BuildValue("(i, {s:K, s:i, s:K})", (int)CfdpFaultInd, "transaction_id",
                                                     (unsigned long long)transaction_id,
                                                     "code", (int)deliveryCode,
                                                     "progress", (unsigned long long)progress);
    }

    // Handle CfdpAbandonedInd
    if (type == CfdpAbandonedInd) {
        cfdp_decompress_number(&transaction_id, &(transactionId.transactionNbr));
        return Py_BuildValue("(i, {s:K, s:i, s:K})", (int)CfdpAbandonedInd, "transaction_id",
                                                     (unsigned long long)transaction_id,
                                                     "code", (int)deliveryCode,
                                                     "progress", (unsigned long long)progress);
    }

    // Handle CfdpFileSegmentRecvInd
    if (type == CfdpFileSegmentRecvInd) {
        cfdp_decompress_number(&transaction_id, &(transactionId.transactionNbr));
        return Py_BuildValue("(i, {s:K, s:K, s:I})", (int)CfdpFileSegmentRecvInd,
                                               "transaction_id", (unsigned long long)transaction_id,
                                               "offset", (unsigned long long)offset,
                                               "length", (unsigned int)length);
    }

    // Define variables
    unsigned char usrmsgBuf[256];
    PyObject*     py_usrmsgs = PyList_New(0);

    // Handle CfdpMetadataRecvInd
    if (type == CfdpMetadataRecvInd) {
        cfdp_decompress_number(&transaction_id, &(transactionId.transactionNbr));
        cfdp_decompress_number(&source_entity_nbr, &(originatingTransactionId.sourceEntityNbr));

        // Get all messages
        while (messagesToUser) {
            // Get next message
            if (cfdp_get_usrmsg(&messagesToUser, usrmsgBuf, (int *)&length) < 0) {
                sprintf(err_msg, "Failed getting user messages, check ion.log.");                     
                PyErr_SetString(PyExc_RuntimeError, err_msg);
                return NULL;
            }

            // If empty message, continue
            if (length == 0) 
                continue;

            // Indicate end of user message
            usrmsgBuf[length] = '\0';

            // Add the user message to the list
            PyList_Append(py_usrmsgs, Py_BuildValue("s", (char *)usrmsgBuf));
        }
        
        // Build the return value
        return Py_BuildValue("(i, {s:K, s:K, s:s, s:s, s:O})", 
                            (int)CfdpMetadataRecvInd, 
                            "transaction_id", (unsigned long long)transaction_id,
                            "source_entity_id", (unsigned long long)source_entity_nbr,
                            "source_file_name", sourceFileNameBuf,
                            "dest_file_name", destFileNameBuf,
                            "user_messages", py_usrmsgs);
    }

    // Define variables
    CfdpAction	action;
	int			status;
	char		firstPathName[256];
	char		secondPathName[256];
	char		msgBuf[256];
    PyObject*   py_fs = PyDict_New();

    // Handle CfdpTransactionFinishedInd
    if (type == CfdpTransactionFinishedInd) {
        cfdp_decompress_number(&transaction_id, &(transactionId.transactionNbr));

        // Get all filestore responses
        while (filestoreResponses) {
            // Get next response
            if (cfdp_get_fsresp(&filestoreResponses, &action, &status, firstPathName, 
                                secondPathName, msgBuf) < 0) {
                sprintf(err_msg, "Failed getting FS response, check ion.log.");                     
                PyErr_SetString(PyExc_RuntimeError, err_msg);
                return NULL;
            }

            // If no action
            if (action == ((CfdpAction) -1))
                continue;

            // Build a value for this action
            PyObject* py_res = PyDict_New();
            PyDict_SetItemString(py_res, "status_report", Py_BuildValue("s", statusReportBuf));
            PyDict_SetItemString(py_res, "condition_code", Py_BuildValue("i", (int)condition));
            PyDict_SetItemString(py_res, "file_status", Py_BuildValue("i", (int)status));
            PyDict_SetItemString(py_res, "delivery_code", Py_BuildValue("i", (int)deliveryCode));

            // Store the result for this action
            PyDict_SetItem(py_fs, Py_BuildValue("i", (int)action), py_res);
        }

        // Build return value
        return Py_BuildValue("(i, O)", (int)CfdpTransactionFinishedInd, py_fs);
    }

    // If you reach this point, you cannot process this event type
    sprintf(err_msg, "Unknown CFDP type.");                     
    PyErr_SetString(PyExc_RuntimeError, err_msg);
    return NULL;
}

static PyObject *pyion_cfdp_interrupt_events(PyObject *self, PyObject *args) {   
    // Interrupt CFDP event handler
    cfdp_interrupt();

    // Return None to indicate success
    Py_RETURN_NONE;
}