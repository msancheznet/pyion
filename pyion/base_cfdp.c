#include <cfdp.h>
#include <bputa.h>

#include "base_cfdp.h"
/**
 * Worker file to handle pure C function
 * interaction between PyION and ION.
 * 
 */

/**
 * 
 * cfdp param initialiation function. Compress the destination eid 
 * from a particular uvast. Then, apply the appropriate 
 * anciallyData flags based off of our given criticality.
 */
int base_cfdp_open(CfdpReqParms *params, uvast from, int criticality)
{
    cfdp_compress_number(&(params->destinationEntityNbr), from);
    if (criticality == 1)
        params->utParms.ancillaryData.flags |= BP_MINIMUM_LATENCY;
    else
        params->utParms.ancillaryData.flags &= (~BP_MINIMUM_LATENCY);

    return 0;
}

int base_cfdp_attach()
{
    return cfdp_attach();
}

void base_cfdp_detach()
{
    return cfdp_detach();
}

int base_cfdp_add_usrmsg(MetadataList list, unsigned char *text)
{
    return cfdp_add_usrmsg(list, text, strlen((const char*) text) + 1);
}

MetadataList base_cfdp_create_usrmsg_list(void)
{
    return cfdp_create_usrmsg_list();
}

MetadataList base_cfdp_create_fsreq_list(void)
{
    return cfdp_create_fsreq_list();
}

int base_cfdp_add_fsreq(MetadataList list, CfdpAction action, char *firstFileName, char *secondFileName)
{
    return cfdp_add_fsreq(list, action, firstFileName, secondFileName);
}

void setParams(CfdpReqParms *params, char *sourceFile, char *destFile,
               int segMetadata, int closureLat, long int mode)
{
    // Fill in basic parameters
    params->sourceFileName = sourceFile;
    params->destFileName = destFile;
    params->segMetadataFn = (segMetadata == 0) ? NULL : noteSegmentTime;
    params->closureLatency = closureLat;

    // mode = 1: Select unreliable CFDP mode
    if (mode & 0x01)
    {
        params->utParms.ancillaryData.flags |= BP_BEST_EFFORT;
        return;
    }

    // mode = 2: Select native BP reliability
    if (mode & 0x02)
    {
        params->utParms.custodySwitch = SourceCustodyRequired;
        return;
    }

    // Mode = 3: Select CL reliability
    params->utParms.ancillaryData.flags &= (~BP_BEST_EFFORT);
    params->utParms.custodySwitch = NoCustodyRequested;
}

int base_cfdp_send(CfdpReqParms *params)
{
    return cfdp_put(&(params->destinationEntityNbr), sizeof(BpUtParms),
                    (unsigned char *)&(params->utParms), params->sourceFileName,
                    params->destFileName, NULL, params->segMetadataFn,
                    NULL, 0, NULL, params->closureLatency, params->msgsToUser,
                    params->fsRequests, &(params->transactionId));
}

/* ============================================================================
 * === Send/Request Functions (and helpers)
 * ============================================================================ */

int noteSegmentTime(uvast fileOffset, unsigned int recordOffset,
                    unsigned int length, int sourceFileFd, char *buffer)
{
    writeTimestampLocal(getCtime(), buffer);
    return strlen(buffer) + 1;
}

int base_cfdp_get(CfdpReqParms *params, CfdpProxyTask *task)
{
    return cfdp_get(&(params->destinationEntityNbr), sizeof(BpUtParms),
                    (unsigned char *)&(params->utParms), NULL, NULL, NULL,
                    NULL, 0, NULL, 0, 0, 0, task, &(params->transactionId));
}

int base_cfdp_cancel(CfdpReqParms *params)
{
    return cfdp_cancel(&(params->transactionId));
}

int base_cfdp_suspend(CfdpReqParms *params)
{
    return cfdp_suspend(&(params->transactionId));
}

int base_cfdp_report(CfdpReqParms *params)
{
    return cfdp_report(&(params->transactionId));
}

void base_cfdp_interrupt(void)
{
    return cfdp_interrupt();
}

void base_cfdp_decompress_number(uvast *toNbr, CfdpNumber *from)
{
    return cfdp_decompress_number(toNbr, from);
}

int base_cfdp_get_usrmsg(MetadataList *list, unsigned char *textBuf, int *length)
{
    return cfdp_get_usrmsg(list, textBuf, length);
}

int base_cfdp_get_fsresp(MetadataList *list, CfdpAction *action,
                         int *status, char *firstFileNameBuf, char *secondFileNameBuf, char *messageBuf)
{
    return cfdp_get_fsresp(list, action, status, firstFileNameBuf,
                           secondFileNameBuf, messageBuf);
}

int base_cfdp_get_event(CfdpEventType *type, time_t *time, 
int *reqNbr, CfdpTransactionId *transactionId, char *sourceFileNameBuf, char *destFileNameBuf, 
uvast *fileSize, MetadataList *messagesToUser, uvast *offset, unsigned int *length, unsigned int 
*recordBoundsRespected, CfdpContinuationState *continuationState, unsigned int *segMetadataLength,
 char *segMetadataBuffer, CfdpCondition *condition, uvast *progress, CfdpFileStatus *fileStatus, 
 CfdpDeliveryCode *deliveryCode, CfdpTransactionId *originatingTransactionId, char *statusReportBuf, 
 MetadataList *filestoreResponses) {
    return cfdp_get_event(type, time, reqNbr, transactionId, sourceFileNameBuf,
    destFileNameBuf, fileSize, messagesToUser, offset, length, recordBoundsRespected,
    continuationState, segMetadataLength, segMetadataBuffer, condition, progress,
    fileStatus, deliveryCode, originatingTransactionId, statusReportBuf, filestoreResponses);
}
