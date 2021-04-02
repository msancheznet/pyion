/* ============================================================================
 * Library to send/receive files via ION's CCSDS File Delivery Protocol
 *
 * Author: Marc Sanchez Net
 * Date:   04/02/2021
 * Copyright (c) 2021, California Institute of Technology ("Caltech").  
 * U.S. Government sponsorship acknowledged.
 * =========================================================================== */

#include "base_cfdp.h"

CfdpReqParms* newCfdpReqParms() {
    CfdpReqParms* params = (CfdpReqParms*)malloc(sizeof(CfdpReqParms));
    if (params == NULL) return params;
    memset(params, 0, sizeof(CfdpReqParms));
    return params;
}

int base_cfdp_attach() {
    return (cfdp_attach() < 0) ? -1 : PYION_OK;
}

int base_cfdp_detach() {
    cfdp_detach();
    return PYION_OK;
}

int base_cfdp_open(CfdpReqParms* params, uvast entityId, int ttl, int classOfService, int ordinal, int srrFlags, int criticality) {
    // Initialize variables
    cfdp_compress_number(&(params->destinationEntityNbr), entityId);
    params->utParms.lifespan = ttl;
	params->utParms.classOfService = classOfService;
    params->utParms.srrFlags = srrFlags;
    params->utParms.ancillaryData.ordinal = ordinal;
    if (criticality == 1)
		params->utParms.ancillaryData.flags |= BP_MINIMUM_LATENCY;
	else
		params->utParms.ancillaryData.flags &= (~BP_MINIMUM_LATENCY);

    return PYION_OK;
}

int base_cfdp_close(CfdpReqParms* params) {
    free(params);
    return PYION_OK;
}

int base_cfdp_add_usr_msg(CfdpReqParms* params, char* usrMsg) {
    // Create user message list if necessary
    if (params->msgsToUser == 0)
        params->msgsToUser = cfdp_create_usrmsg_list();

    // Add user message
    oK(cfdp_add_usrmsg(params->msgsToUser, (unsigned char *)usrMsg, strlen(usrMsg)+1));

    return PYION_OK;
}

int base_cfdp_add_fs_req(CfdpReqParms* params, CfdpAction action, char* firstPathName, char* secondPathName) {
    // Create a file request list if necessary
    if (params->fsRequests == 0)
        params->fsRequests = cfdp_create_fsreq_list();

    // Add file request
    oK(cfdp_add_fsreq(params->fsRequests, action, firstPathName, secondPathName));

    return PYION_OK;
}

int	noteSegmentTime(uvast fileOffset, unsigned int recordOffset,
			        unsigned int length, int sourceFileFd, char *buffer) {
	writeTimestampLocal(getCtime(), buffer);
	return strlen(buffer) + 1;
}

void setParams(CfdpReqParms *params, char *sourceFile, char *destFile, 
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

int base_cfdp_send(CfdpReqParms* params, char* sourceFile, char* destFile, int closureLat, int segMetadata, long int mode) {
    // Define variables
    int ok;

    // Store parameters
    setParams(params, sourceFile, destFile, segMetadata, closureLat, mode);

    // Trigger the CFDP put
    ok = cfdp_put(&(params->destinationEntityNbr), sizeof(BpUtParms), 
                 (unsigned char *) &(params->utParms), params->sourceFileName,
				 params->destFileName, NULL, params->segMetadataFn,
				 NULL, 0, NULL, params->closureLatency, params->msgsToUser,
				 params->fsRequests, &(params->transactionId));

    // Handle error
    if (ok < 0) return -1;

    // Reset parameters
    params->msgsToUser = 0;
    params->fsRequests = 0;

    return PYION_OK;
}

int base_cfdp_request(CfdpReqParms* params, char* sourceFile, char* destFile, int closureLat, int segMetadata, long int mode) {
    // Define variables
    int ok;
    CfdpProxyTask task;

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
    ok = cfdp_get(&(params->destinationEntityNbr), sizeof(BpUtParms),
					(unsigned char *) &(params->utParms), NULL, NULL, NULL, 
                    NULL, 0, NULL, 0, 0, 0, &task, &(params->transactionId));

    // Handle error
    if (ok < 0) return -1;

    // Reset parameters
    params->msgsToUser = 0;
    params->fsRequests = 0;

    return PYION_OK;                    
}

int base_cfdp_cancel(CfdpReqParms* params) {
    return (cfdp_cancel(&(params->transactionId)) < 0) ? -1 : PYION_OK;
}

int base_cfdp_suspend(CfdpReqParms* params) {
    return (cfdp_suspend(&(params->transactionId)) < 0) ? -1 : PYION_OK;
}

int base_cfdp_resume(CfdpReqParms* params) {
    return (cfdp_resume(&(params->transactionId)) < 0) ? -1 : PYION_OK;
}

int base_cfdp_report(CfdpReqParms* params) {
    return (cfdp_report(&(params->transactionId)) < 0) ? -1 : PYION_OK;
}

int base_cfdp_next_event(CfdpEventInfo* info) {
    // Define variables
    int ok;

    // Get the next event
    ok = cfdp_get_event(&(info->type), &(info->time), &(info->reqNbr), 
                        &(info->transactionId), info->sourceFileNameBuf, 
                        info->destFileNameBuf, &(info->fileSize),
                        &(info->messagesToUser), &(info->offset),
                        &(info->length), &(info->recordBoundsRespected), 
                        &(info->continuationState), &(info->segMetadataLength),
                        info->segMetadata, &(info->condition),
                        &(info->progress), &(info->fileStatus),
                        &(info->deliveryCode), &(info->originatingTransactionId),
                        info->statusReportBuf, &(info->filestoreResponses));

    return (ok < 0) ? -1: PYION_OK;                
}

int base_cfdp_interrupt_events() {
    cfdp_interrupt();
    return PYION_OK;
}