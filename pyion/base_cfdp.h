/**
 * Shim-layer 
 */
#include <cfdp.h>
#include <bputa.h>

typedef struct
{
    CfdpHandler faultHandlers[16];
    CfdpNumber destinationEntityNbr;
    char *sourceFileName;
    char destFileNameBuf[256];
    char *destFileName;
    BpUtParms utParms;
    unsigned int closureLatency;
    CfdpMetadataFn segMetadataFn;
    MetadataList msgsToUser;
    MetadataList fsRequests;
    CfdpTransactionId transactionId;
} CfdpReqParms;

int base_cfdp_attach();

void base_cfdp_detach();

int base_cfdp_compress_number();

int base_cfdp_open();

MetadataList base_cfdp_create_usrmsg_list();

void setParams(CfdpReqParms *params, char *sourceFile, char *destFile, 
                      int segMetadata, int closureLat, long int mode);

int	noteSegmentTime(uvast fileOffset, unsigned int recordOffset,
			unsigned int length, int sourceFileFd, char *buffer);

int base_cfdp_get();

int base_cfdp_add_usrmsg(MetadataList list, unsigned char *text);

MetadataList base_cfdp_create_fsreq_list(void);

int base_cfdp_add_fsreq(MetadataList list, CfdpAction action, char *firstFileName, char *secondFileName);

int base_cfdp_cancel(CfdpReqParms *params);

int base_cfdp_suspend(CfdpReqParms *params);

int base_cfdp_report(CfdpReqParms *params);

void base_cfdp_interrupt(void);



void base_cfdp_decompress_number(uvast *toNbr, CfdpNumber *from);

int base_cfdp_get_usrmsg(MetadataList *list, unsigned char *textBuf, int *length);

int base_cfdp_get_fsresp(MetadataList *list, CfdpAction *action,
                         int *status, char *firstFileNameBuf, char *secondFileNameBuf, char *messageBuf);

int base_cfdp_get_event(CfdpEventType *type, time_t *time, 
int *reqNbr, CfdpTransactionId *transactionId, char *sourceFileNameBuf, char *destFileNameBuf, 
uvast *fileSize, MetadataList *messagesToUser, uvast *offset, unsigned int *length, unsigned int 
*recordBoundsRespected, CfdpContinuationState *continuationState, unsigned int *segMetadataLength,
 char *segMetadataBuffer, CfdpCondition *condition, uvast *progress, CfdpFileStatus *fileStatus, 
 CfdpDeliveryCode *deliveryCode, CfdpTransactionId *originatingTransactionId, char *statusReportBuf, 
 MetadataList *filestoreResponses);