/* ============================================================================
 * Library to send/receive data via ION's Licklider Transmission Protocol
 *
 * Author: Marc Sanchez Net
 * Date:   03/22/2021
 * Copyright (c) 2021, California Institute of Technology ("Caltech").  
 * U.S. Government sponsorship acknowledged.
 * =========================================================================== */

#ifndef BASELTP_H
#define BASELTP_H

#include <ltp.h>

// Max number of allowed LTP sessions
#define MAX_LTP_SESSIONS 1024

// [bytes] Size of the pre-allocated buffer to receive bundles
// NOTE: This avoids having to malloc when receiving small bundles
#define MAX_PREALLOC_BUFFER 1024

// Possible states of an LTP service access point
typedef enum {
    SAP_IDLE = 0,
    SAP_RUNNING,
    SAP_CLOSING
} LtpStateEnum;

// State of the LTP service access point
typedef struct {
    unsigned int clientId;      // 1=BP, 2=SDA, 3=CFDP, other numbers available
    LtpStateEnum status;
} LtpSAP;

// Struct used to receive data through an enpoint. It contains a 
// preallocated buffer to speed things up.
typedef struct
{
    int len;
    int do_malloc;
    char *payload;
    char payload_prealloc[MAX_PREALLOC_BUFFER];
} RxPayload;

// Attach to ION's BP engine
int base_ltp_attach();

// Detach from ION's BP engine
int base_ltp_detach();

// Open LTP access point
int base_ltp_open(LtpSAP **state_ref, unsigned int clientId);

// Close LTP access point
int base_ltp_close(LtpSAP* state);

// Send data via LTP access point
int base_ltp_send(LtpSAP *state, unsigned long long destEngineId, char* data, int data_size);

// Interrupt LTP access point
int base_ltp_interrupt(LtpSAP *state);

// Receive data via an LTP access point
int base_ltp_receive(LtpSAP* state, RxPayload* msg);

#endif
