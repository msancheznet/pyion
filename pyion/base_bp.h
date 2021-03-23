/* ============================================================================
 * Library to send/receive data via ION's Bundle Protocol
 *
 * Author: Marc Sanchez Net
 * Date:   03/22/2021
 * Copyright (c) 2021, California Institute of Technology ("Caltech").  
 * U.S. Government sponsorship acknowledged.
 * =========================================================================== */

#ifndef BASEBP_H
#define BASEBP_H

#include <bp.h>

// [bytes] Size of the pre-allocated buffer to receive bundles
// NOTE: This avoids having to malloc when receiving small bundles
#define MAX_PREALLOC_BUFFER 1024

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

// Struct used to receive data through an enpoint. It contains a 
// preallocated buffer to speed things up.
typedef struct
{
    int len;
    int do_malloc;
    char *payload;
    char payload_prealloc[MAX_PREALLOC_BUFFER];
} RxPayload;

// Struct to encapsuate all input parameters needed when transmitting
// a bundle via ION's implementation of the Bundle Protocol
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

// Attach to ION's BP engine
int base_bp_attach();

// Detach from ION's BP engine
int base_bp_detach();

// Close a BP endpoint
void base_close_endpoint(BpSapState *state);

// Open an endpoint
int base_bp_open(BpSapState **state, char *ownEid, int detained, int mem_ctrl);

// Send data via an endpoint
int base_bp_send(BpSapState *state, TxPayload *txInfo);

// Receive data via an endpoint
int base_bp_receive_data(BpSapState *state, RxPayload *msg);

// Interrupt the reception of data via an endpoint
int base_bp_interrupt(BpSapState *state);


#endif
