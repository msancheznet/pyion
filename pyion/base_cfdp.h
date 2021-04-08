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
