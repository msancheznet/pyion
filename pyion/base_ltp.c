/* ============================================================================
 * Library to send/receive data via ION's Bundle Protocol
 *
 * Author: Marc Sanchez Net
 * Date:   04/01/2021
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
#include <ltp.h>

#include "base_ltp.h"
#include "_utils.c"
#include "macros.h"

/* ============================================================================
 * === ATTACH/DETACH 
 * ============================================================================ */

int base_ltp_attach() {
    return ltp_attach();
}

int base_ltp_detach() {
    ltp_detach();
    return PYION_OK;
}

/* ============================================================================
 * === OPEN/CLOSE ACCESS POINT
 * ============================================================================ */

int base_ltp_open(LtpSAP **state_ref, unsigned int clientId) {
    // Define variables
    int ok;

    // malloc space for bp sap state
    *state_ref = (LtpSAP*)malloc(sizeof(LtpSAP));

    // Initialize state
    LtpSAP *state = *state_ref;
    if (state == NULL) return -1;
    memset((char *)state, 0, sizeof(LtpSAP));

    // Open connection to LTP client
    ok = ltp_open(clientId);

    // If error return
    if (ok < 0) return -2;

    // Fill state information
    state->clientId = clientId;
    state->status = SAP_IDLE;

    // Return success
    return PYION_OK;
}

void close_access_point(LtpSAP *state) {
    // Close this SAP
    ltp_close(state->clientId);

    // Free state memory
    free(state);
}

int base_ltp_close(LtpSAP* state) {
    // Initialize variables
    int ok;

    // If endpoint is in idle state, just close
    if (state->status == SAP_IDLE) {
        close_access_point(state);
        return PYION_OK;
    }

    // We assume that if you reach this point, you are always in 
    // running state.
    state->status = SAP_CLOSING;
    ltp_interrupt(state->clientId);

    return PYION_OK;
}

/* ============================================================================
 * === SEND DATA
 * ============================================================================ */

int base_ltp_send(LtpSAP *state, unsigned long long destEngineId, char* data, int data_size) {
    // Define variables
    LtpSessionId sessionId;
    Sdr          sdr;
    Object       extent;
    Object		 item = 0;

    // Get ION SDR
    sdr = getIonsdr();

    // Allocate SDR memory
    SDR_BEGIN_XN
    extent = sdr_insert(sdr, data, (size_t)data_size);
    if (!extent) {
        sdr_cancel_xn(sdr);
        return PYION_SDR_ERR;
    }
    SDR_END_XN

    // Create ZCO object (not blocking because there is no attendant)
    item = ionCreateZco(ZcoSdrSource, extent, 0, data_size,
                        0, 0, ZcoOutbound, NULL); 
    ZCO_CREATE_ERROR(item)                        

    // Send using LTP protocol. All data is sent as RED LTP by definition.
    // NOTE 1: SessionId is filled by ``ltp_send``, but you do not care about it
    int ok = ltp_send((uvast)destEngineId, state->clientId, item, LTP_ALL_RED, &sessionId);

    // Handle error in ltp_send
    if (ok <= 0) return PYION_IO_ERR;    
    
    return PYION_OK;
}

/* ============================================================================
 * === INTERRUPT ACCESS POINT
 * ============================================================================ */

int base_ltp_interrupt(LtpSAP* state) {
    // If access point is already closing, you can return
    if (state->status != SAP_RUNNING)
        return PYION_OK;

    // Mark that you have transitioned to interruping state
    state->status = SAP_CLOSING;
    ltp_interrupt(state->clientId);

    return PYION_OK;
}

/* ============================================================================
 * === RECEIVE VIA ACCESS POINT
 * ============================================================================ */

int help_receive_data(LtpSAP *state, RxPayload *msg){
    // Define variables
    ZcoReader	    reader;
    Sdr             sdr;
    LtpNoticeType	type;
	LtpSessionId	sessionId;
	unsigned char	reasonCode;
	unsigned char	endOfBlock;
	unsigned int	dataOffset;
	unsigned int	dataLength;
	Object		    data;
    int             receiving_block, notice, do_malloc;
    vast            len, data_size;

    // Mark as running
    receiving_block = 1;
    state->status = SAP_RUNNING;

    // Process incoming indications
    while ((state->status == SAP_RUNNING) && (receiving_block == 1)) {
        // Get the next LTP notice
        notice = ltp_get_notice(state->clientId, &type, &sessionId, &reasonCode, 
                                &endOfBlock, &dataOffset, &dataLength, &data);

        // Handle error while receiving notices
        if (notice < 0) return PYION_IO_ERR;

        // Handle different notice types
        switch (type) {
            case LtpExportSessionComplete:      // Transmit success
                // Do not mark reception of block complete here. This should be done by the 
                // LtpRecvRedPart case.
                break;
            case LtpImportSessionCanceled:      // Cancelled sessions. No data has been received yet.
                // Release any data and throw exception
                ltp_release_data(data);
                return -1;
            case LtpExportSessionCanceled:      // Transmit failure. Some data might have been received already.
                // Release any data and throw exception
                ltp_release_data(data);
                return -2;
            case LtpRecvRedPart:
                // If this is not the end of the block, you are dealing with a block that is
                // partially green, partially red. This is not allowed.
                if (!endOfBlock) {
                    ltp_release_data(data);
                    return -3;
                }
                
                // Mark that reception of block has ended
                receiving_block = 0;
                break;
            case LtpRecvGreenSegment:
                // Release any data and throw exception
                ltp_release_data(data);
                return -4;
            default:
                break;
        }

        // Make sure other tasks have a chance to run
        sm_TaskYield();
    }

    // If you exited because of closing, throw error
    if (state->status == SAP_CLOSING) return PYION_CONN_ABORTED_ERR;

    // If no block received by now and you are not closing, error.
    if (receiving_block == -1) return PYION_IO_ERR;

    // Get ION SDR
    sdr = getIonsdr();

    // Get content data size
    SDR_BEGIN_XN
    data_size = zco_source_data_length(sdr, data);
    SDR_EXIT_XN

    // Check if we need to allocate memory dynamically
    do_malloc = (data_size > 1024);

    // Allocate memory for payload if necessary
    char* payload = do_malloc ? (char*)malloc(data_size) : msg->payload_prealloc;

    // Prepare to receive next block
    zco_start_receiving(data, &reader);

    // Get bundle data
    SDR_BEGIN_XN
    len = zco_receive_source(sdr, &reader, data_size, payload);
    msg->payload = payload;
    msg->len = len;
    msg->do_malloc = do_malloc;
    SDR_END_XN

    // Handle error while getting the payload
    if (len < 0) {
        if (do_malloc) free(payload);
        return -5;
    }

    // Release LTP object now that you are done with it.
    ltp_release_data(data);

    return PYION_OK;
}

int base_ltp_receive(LtpSAP* state, RxPayload* msg) {
    // Trigger reception of data
    int ok = help_receive_data(state, msg);

    // Close if necessary. Otherwise set to IDLE
    if (state->status == SAP_CLOSING) {
        close_access_point(state);
    } else {
        state->status = SAP_IDLE;
    }

    // Return value
    return ok;
}