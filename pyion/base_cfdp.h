/* ============================================================================
 * Library to send/receive files via ION's CCSDS File Delivery Protocol
 *
 * Author: Marc Sanchez Net
 * Date:   04/02/2021
 * Copyright (c) 2021, California Institute of Technology ("Caltech").  
 * U.S. Government sponsorship acknowledged.
 * =========================================================================== */

#ifndef BASECFDP_H
#define BASECFDP_H

#include <cfdp.h>
#include <bputa.h>

#include "macros.h"
#include "_utils.c"

// Structure to store all the information necessary to make a request via CFDP
typedef struct {
	CfdpHandler		    faultHandlers[16];
	CfdpNumber		    destinationEntityNbr;
	char			    *sourceFileName;
	char			    destFileNameBuf[256];
	char			    *destFileName;
	BpUtParms		    utParms;
	unsigned int		closureLatency;
	CfdpMetadataFn		segMetadataFn;
	MetadataList		msgsToUser;
	MetadataList		fsRequests;
	CfdpTransactionId	transactionId;
} CfdpReqParms;

// Structure to store the output of a cfdp event
typedef struct {
    CfdpEventType type;
    time_t                  time;
    int                     reqNbr;
    CfdpTransactionId       transactionId;
	char                    sourceFileNameBuf[256];
	char                    destFileNameBuf[256];
    uvast			        fileSize;
	MetadataList		    messagesToUser;
	uvast			        offset;
	unsigned int		    length;
	unsigned int		    recordBoundsRespected;
	CfdpContinuationState	continuationState;
	unsigned int		    segMetadataLength;
	char			        segMetadata[63];
	CfdpCondition		    condition;
	uvast			        progress;
	CfdpFileStatus		    fileStatus;
	CfdpDeliveryCode	    deliveryCode;
	CfdpTransactionId	    originatingTransactionId;
	char			        statusReportBuf[256];
	MetadataList		    filestoreResponses;
} CfdpEventInfo;

// Constructor for the CfdpReqParms structure
CfdpReqParms* newCfdpReqParms();

// Attach to CFDP
int base_cfdp_attach();

// Detach from CFDP
int base_cfdp_detach();

// Open CFDP entity
int base_cfdp_open(CfdpReqParms* params, uvast entityId, int ttl, int classOfService,
                   int ordinal, int srrFlags, int criticality);

// Close CFDP entity
int base_cfdp_close(CfdpReqParms* params);

// Add a user message to a transaction
int base_cfdp_add_usr_msg(CfdpReqParms* params, char* usrMsg);

// Add a file store request
int base_cfdp_add_fs_req(CfdpReqParms* params, CfdpAction action, char* firstPathName, char* secondPathName);

// Send file via CFDP
int base_cfdp_send(CfdpReqParms* params, char* sourceFile, char* destFile, int closureLat, int segMetadata, long int mode);

// Request file via CFDP
int base_cfdp_request(CfdpReqParms* params, char* sourceFile, char* destFile, int closureLat, int segMetadata, long int mode);

// Cancel a CFDP transaction
int base_cfdp_cancel(CfdpReqParms* params);

// Suspend a CFDP transaction
int base_cfdp_suspend(CfdpReqParms* params);

// Resume a CFDP transaction
int base_cfdp_resume(CfdpReqParms* params);

// Report on a CFDP transaction
int base_cfdp_report(CfdpReqParms* params);

// Get next CFDP event
int base_cfdp_next_event();

// Interrupt CFDP event handler
int base_cfdp_interrupt_events();

#endif
