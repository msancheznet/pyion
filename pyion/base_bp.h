#ifndef BASEBP_H
#define BASEBP_H

#include <bp.h>

#define MAX_PREALLOC_BUFFER 1024

/**
 * Shim-layer 
 */

// Possible states of an enpoint. This is used to avoid race conditions
// when closing receiving threads.
typedef enum
{
    EID_IDLE = 0,
    EID_RUNNING,
    EID_CLOSING,
    EID_INTERRUPTING
} SapStateEnum;


// A combination of a BpSAP object and a representation of its status.
// The status only used during reception for now.
typedef struct
{
    BpSAP sap;
    SapStateEnum status;
    int detained;
    ReqAttendant *attendant;
} BpSapState;


/**
 * Struct designed to contain or reference received message information.
 * To avoid calls to malloc, we can preallocate a buffer on the stack to
 * contain the received message. If this buffer is of insufficient size,
 * Then it is up to us to dynamically allocate *payload and free it when
 * necessary. 
 */
typedef struct
{
    int len;
    int do_malloc;
    char payload_prealloc[MAX_PREALLOC_BUFFER];
    char *payload; //payload should be initialized to point to payload_prealloc.
    //If we must malloc, remember to set do_malloc to 1
} BpRx;

/**
 * Struct designed to reference data to pass to functions involved
 * in sending messages. The user must provide memory management for
 * the strings.*/
typedef struct
{
    char *data;
    char *destEid;
    char *reportEid;
    int ttl;
    int classOfService;
    int custodySwitch;
    int rrFlags;
    int ackReq;
    unsigned int retxTimer;
    BpAncillaryData *ancillaryData;
    int data_size;

} BpTx;

/** The following "shim-layer" functions call their respective
 * counterparts in ION while handling state information, so that
 * the user of this API does not have to themself.
 *
 * In addition, the various paramaters needed for message reception,
 * transmission, and memory management are encapulated into BpRx and 
 * BpTx.
 */

/**
 * Calls bp_attach, returns result
 */
int base_bp_attach();

/**
 * Calls bp_detach, which is a void function
 */
void base_bp_detach();

void base_close_endpoint(BpSapState *state);

int base_bp_interrupt(BpSapState *state);

int base_bp_receive_data(BpSapState *state, BpRx *msg);

int base_bp_send(BpSapState *state, BpTx *txInfo);

int base_bp_open(BpSapState **state, char *ownEid, int detained, int mem_ctrl);



/**
 * Helper "constructor" functions for BpTx and BpRx objects
 */


int base_heap_create_bp_tx_payload(BpTx **obj); 
//Remember to pass pointer reference to BpTx
//(we are modifying it afterall)

//Free bpTx that was created with base_heap_create_bp_tx_payload()
int base_heap_destroy_bp_tx_payload(BpTx *obj);

//Initialize already-created payload
int base_init_bp_tx_payload(BpTx *obj)
;
//Free payload members without freeing actual payload object
int base_stack_destroy_bp_tx_payload(BpTx *obj);

#endif
