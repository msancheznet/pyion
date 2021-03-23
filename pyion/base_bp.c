/* ============================================================================
 * Library to send/receive data via ION's Bundle Protocol
 *
 * Author: Marc Sanchez Net
 * Date:   03/22/2021
 * Copyright (c) 2021, California Institute of Technology ("Caltech").  
 * U.S. Government sponsorship acknowledged.
 * =========================================================================== */

/**
 * Worker file to handle pure C function
 * interaction between PyION and ION.
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <bp.h>

#include "base_bp.h"
#include "_utils.c"
#include "macros.h"

/* ============================================================================
 * === ATTACH/DETACH 
 * ============================================================================ */

int base_bp_attach() {
    int result = bp_attach();
    return result;
}

int base_bp_detach() {
    bp_detach();
    return PYION_OK;
}

/* ============================================================================
 * === OPEN/CLOSE ENDPOINT
 * ============================================================================ */

int base_bp_open(BpSapState **state_ref, char *ownEid, int detained, int mem_ctrl) {
    // Define variables
    int ok;
    
    // malloc space for bp sap state
    *state_ref = (BpSapState*)malloc(sizeof(BpSapState));

    // Initialize state
    BpSapState *state = *state_ref;
    if (state == NULL) {
        return -1;
    }
    memset((char *)state, 0, sizeof(BpSapState));

    // Open the endpoint. This call fills out the SAP information
    // NOTE: An endpoint must be opened in detained mode if you want
    //       to have custody-based re-tx.
    if (detained == 0) {
        ok = bp_open(ownEid, &(state->sap));
    } else {
        ok = bp_open_source(ownEid, &(state->sap), 1);
    }

    // Handle error while opening endpoint
    if (ok < 0) return -2;

    // Mark the SAP state for this endpoint as running
    state->status   = EID_IDLE;
    state->detained = (detained > 0);

    if (mem_ctrl) {
        // Allocate memory for new attendant
        state->attendant = (ReqAttendant*)malloc(sizeof(ReqAttendant));
        
        // Initialize the attendant
        if (ionStartAttendant(state->attendant)) {
            return -3;
        }
    } else {
        state->attendant = NULL;
    }

    return PYION_OK;
}

void base_close_endpoint(BpSapState *state) {
    // Close and free attendant
    if (state->attendant) {
        ionStopAttendant(state->attendant);
        free(state->attendant);
    } 

    // Close this SAP
    bp_close(state->sap);

    // Free state memory
    free(state);
}

/* ============================================================================
 * === SEND DATA
 * ============================================================================ */

int base_bp_send(BpSapState *state, TxPayload *txInfo)
{
    // Initialize variables
    Object newBundle;
    Sdr sdr = NULL;
    Object bundleZco;
    Object bundleSdr;
    int ok;

    // Get SDR
    sdr = bp_get_sdr();

    // Insert data to SDR
    SDR_BEGIN_XN
    bundleSdr = sdr_insert(sdr, txInfo->data, (size_t)txInfo->data_size);
    SDR_END_XN

    // Create ZCO object
    bundleZco = ionCreateZco(ZcoSdrSource, bundleSdr, 0, txInfo->data_size,
                             txInfo->classOfService, 0, ZcoOutbound, state->attendant);
    ZCO_CREATE_ERROR

    // Send bundle via ION
    ok = bp_send(state->sap, txInfo->destEid, txInfo->reportEid, txInfo->ttl, 
                txInfo->classOfService, txInfo->custodySwitch, txInfo->rrFlags, 
                txInfo->ackReq, txInfo->ancillaryData, bundleZco, &newBundle);

    // Handle error in bp_send
    if (ok <= 0) return PYION_IO_ERR;

    // If you want custody transfer and have specified a re-transmission timer,
    // then activate it
    if (txInfo->custodySwitch == SourceCustodyRequired && txInfo->retxTimer > 0)
    {
        // Note: The timer starts as soon as bp_memo is called.
        ok = bp_memo(newBundle, txInfo->retxTimer);

        // Handle error in bp_memo
        if (ok < 0) {
            return PYION_ERR;
        }
    }

    // If you have opened this endpoint in detained mode, you need to release the bundle
    if (state->detained)
        bp_release(newBundle);

    return PYION_OK;
}

/* ============================================================================
 * === INTERRUPT ENDPOINT
 * ============================================================================ */

int base_bp_interrupt(BpSapState *state) {
    bp_interrupt(state->sap);
     // Pause the attendant
    if (state->attendant) ionPauseAttendant(state->attendant);
    return 0;
}

/* ============================================================================
 * === RECEIVE DATA
 * ============================================================================ */

int help_receive_data(BpSapState *state, BpDelivery *dlv, RxPayload *msg){
    // Define variables
    int data_size, len, rx_ret, do_malloc;
    Sdr sdr;
    ZcoReader reader;

    // Define variables to store the bundle payload. If payload size is less than
    // MAX_PREALLOC_BUFFER, then use preallocated buffer to save time. Otherwise,
    // call malloc to allocate as much memory as you need.
    char *payload;

    // Get ION's SDR
    sdr = bp_get_sdr();

    while (state->status == EID_RUNNING) {
        // Receive data from ION
        rx_ret = bp_receive(state->sap, dlv, BP_BLOCKING);

        // Check if error while receiving a bundle
        if ((rx_ret < 0) && (state->status == EID_RUNNING)) {
            return PYION_IO_ERR;
        }

        // If dlv is not interrupted (e.g., it was successful), get out of loop.
        // From Scott Burleigh: BpReceptionInterrupted can happen because SO triggers an
        // interruption without the user doing anything. Therefore, bp_receive always
        // needs to be enclosed in this type of while loops.
        if (dlv->result != BpReceptionInterrupted)
            break;
    }

    // If you exited because of interruption
    if (state->status == EID_INTERRUPTING)
        return PYION_INTERRUPTED_ERR;

    // If you exited because of closing
    if (state->status == EID_CLOSING)
        return  PYION_CONN_ABORTED_ERR;

    // If endpoint was stopped, finish
    if (dlv->result == BpEndpointStopped)
        return  PYION_CONN_ABORTED_ERR;

    // If bundle does not have the payload, raise IOError
    if (dlv->result != BpPayloadPresent)
        return PYION_IO_ERR;

    // Get content data size
    SDR_BEGIN_XN
    data_size = zco_source_data_length(sdr, dlv->adu);
    SDR_EXIT_XN

    // Check if we need to allocate memory dynamically
    do_malloc = (data_size > MAX_PREALLOC_BUFFER);

    // Allocate memory if necessary
    payload = do_malloc ? (char *)malloc(data_size) : msg->payload_prealloc;

    // Initialize reader
    zco_start_receiving(dlv->adu, &reader);

    // Get bundle data
    SDR_BEGIN_XN
    len = zco_receive_source(sdr, &reader, data_size, payload);
    msg->payload = payload;
    msg->len = len;
    msg->do_malloc = do_malloc;

    // Handle error while getting the payload
    if (sdr_end_xn(sdr) < 0 || len < 0) {
        // Clean up tasks
        if (do_malloc) free(payload);
        return PYION_IO_ERR;
    }

    return PYION_OK;
}


int base_bp_receive_data(BpSapState *state, RxPayload *msg) {
    // Initialize variables
    BpDelivery dlv;

    // Receive data
    int status = help_receive_data(state, &dlv, msg);

    // Release memory
    bp_release_delivery(&dlv, 1);

    // Close if necessary. Otherwise set to IDLE
    if (state->status == EID_CLOSING) {
        base_close_endpoint(state);
    } else {
        state->status = EID_IDLE;
    }

    return status;
}
