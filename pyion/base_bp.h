#ifndef BASEBP_H
#define BASEBP_H

#include <bp.h>

#define MAX_PREALLOC_BUFFER 1024

/**
 * Direcctly calls ION bp_attach().. 
 * Returns the return value of bp_attach.
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

typedef struct
{
    int len;
    int do_malloc;
    char *payload;
    char payload_prealloc[MAX_PREALLOC_BUFFER];
} RxPayload;


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

} TxPayload;


int base_bp_attach();

int base_bp_detach();

void base_close_endpoint(BpSapState *state);

int base_bp_interrupt(BpSapState *state);

int base_bp_receive_data(BpSapState *state, RxPayload *msg);

//TODO: Utilize some struct
int base_bp_send(BpSapState *state, TxPayload *txInfo);

int base_bp_open(BpSapState **state, char *ownEid, int detained, int mem_ctrl);

//Create a payload on the heap
int base_heap_create_bp_tx_payload(TxPayload **obj);

//Free a payload on the heap
int base_heap_destroy_bp_tx_payload(TxPayload *obj);

//Initialize already-created payload
int base_init_bp_tx_payload(TxPayload *obj);

//Free payload members without freeing actual payload object
int base_stack_destroy_bp_tx_payload(TxPayload *obj);

#endif
