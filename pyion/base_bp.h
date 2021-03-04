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
    char *payload;
    int len;
    int do_malloc;
} RxPayload;



int base_bp_attach();

int base_bp_detach();

void base_close_endpoint(BpSapState *state);

int base_bp_interrupt(BpSapState *state);

int base_bp_receive_data(BpSapState *state, RxPayload *msg);


//TODO: Utilize some struct
int base_bp_send(char *destEid, char *reportEid, int ttl, int classOfService,
                 int custodySwitch, int rrFlags, int ackReq, unsigned int retxTimer,
                 BpAncillaryData *ancillaryData, int data_size);

int base_bp_open(BpSapState *state, int mem_ctrl);

#endif