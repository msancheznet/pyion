/**
 * Worker file to handle pure C function
 * interaction between PyION and ION.
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <bp.h>
#include "return_codes.h"
#include "base_bp.h"
#include "_utils.c"



int base_bp_attach() {
    

    
    int result = bp_attach();
    return result;
}


void base_bp_detach() {
    return bp_detach();
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

int base_bp_interrupt(BpSapState *state) {
    bp_interrupt(state->sap);
     // Pause the attendant
    if (state->attendant) ionPauseAttendant(state->attendant);
    return 0;
}

int base_create_bp_tx_payload(BpTx **obj) {
    BpTx *txInfo = malloc(sizeof(BpTx));
    txInfo->destEid = NULL;
    txInfo->reportEid = NULL;
    txInfo->ttl = 0;
    txInfo->classOfService = 0;
    txInfo->custodySwitch = 0;
    txInfo->rrFlags = 0;
    txInfo->ackReq = 0;
    txInfo->retxTimer = 0;
    txInfo->data = NULL;
    txInfo->data_size = 0;
    txInfo->ancillaryData = NULL;
    *obj = txInfo;
}

int base_init_bp_tx_payload(BpTx *txInfo) {
    txInfo->destEid = NULL;
    txInfo->reportEid = NULL;
    txInfo->ttl = 0;
    txInfo->classOfService = 0;
    txInfo->custodySwitch = 0;
    txInfo->rrFlags = 0;
    txInfo->ackReq = 0;
    txInfo->retxTimer = 0;
    txInfo->data = NULL;
    txInfo->data_size = 0;
    txInfo->ancillaryData = NULL;
    return 0;
}

int base_stack_destroy_bp_tx_payload(BpTx *obj) {
    return 0;
}

int base_heap_destroy_bp_tx_payload(BpTx *obj) {
   return 0;
}


int help_receive_data(BpSapState *state, BpDelivery *dlv, BpRx *msg){

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
    CHKZERO(sdr); //asert statement, checking SDR
    CHKZERO(dlv);
    CHKZERO(msg);

    while (state->status == EID_RUNNING) {
        rx_ret = bp_receive(state->sap, dlv, BP_BLOCKING);
                                    // Acquire the GIL
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
    if (state->status == EID_INTERRUPTING) {
        return PYION_INTERRUPTED_ERR;
    }

    // If you exited because of closing
    if (state->status == EID_CLOSING) {
        return  PYION_CONN_ABORTED_ERR;
    }

    // If endpoint was stopped, finish
    if (dlv->result == BpEndpointStopped) {
        return  PYION_CONN_ABORTED_ERR;
    }

    // If bundle does not have the payload, raise IOError
    if (dlv->result != BpPayloadPresent) {
        return PYION_IO_ERR;
    }

    // Get content data size
    if (!sdr_pybegin_xn(sdr)) return PYION_SDR_ERR;
    data_size = zco_source_data_length(sdr, dlv->adu);
    sdr_pyexit_xn(sdr);

    // Check if we need to allocate memory dynamically
    do_malloc = (data_size > MAX_PREALLOC_BUFFER);

    // Allocate memory if necessary
    payload = do_malloc ? (char *)malloc(data_size) : msg->payload_prealloc;

    // Initialize reader
    zco_start_receiving(dlv->adu, &reader);

    // Get bundle data
    if (!sdr_pybegin_xn(sdr)) return PYION_SDR_ERR;
    len = zco_receive_source(sdr, &reader, data_size, payload);

    msg->payload = payload;
    msg->len = len;
    msg->do_malloc = do_malloc;

    // Handle error while getting the payload
    if (sdr_end_xn(sdr) < 0 || len < 0) {
        // Clean up tasks
        if (do_malloc) free(payload);
        return PYION_IO_ERR;
        //return NULL;
    }
    return 0;
}


int base_bp_receive_data(BpSapState *state, BpRx *msg) {
    BpDelivery dlv;

    int status = help_receive_data(state, &dlv, msg);
    bp_release_delivery(&dlv,1);

    // Close if necessary. Otherwise set to IDLE
    if (state->status == EID_CLOSING) {
        base_close_endpoint(state);
    } else {
        state->status = EID_IDLE;
    }

    return status;
}


int base_bp_open(BpSapState **state_ref, char *ownEid, int detained, int mem_ctrl) {
        // Define variables
    int ok;
    
    //malloc space for bp sap state
    *state_ref = (BpSapState*)malloc(sizeof(BpSapState));

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

return 0;


}



/**
 * 
 * Base-level function to send a bundle through
 * ION's bp.h library.
 * 
 */
/*destEid, reportEid, ttl, classOfService, 
           custodySwitch, rrFlags, ackReq, ancillaryData, 
           bundleZco, &newBundle*/
int base_bp_send(BpSapState *state, BpTx *txInfo)
{

    Object newBundle;
    Sdr sdr = NULL;
    Object bundleZco;
    Object bundleSdr;
    int ok;
   

    // Initialize variables
    sdr = bp_get_sdr();
    //printf("THIS IS SOMETHING THAT I ADDED!");

    // Insert data to SDR
    if (!sdr_pybegin_xn(sdr))
        return PYION_SDR_ERR;
    bundleSdr = sdr_insert(sdr, txInfo->data, (size_t)txInfo->data_size);
    if (!sdr_pyend_xn(sdr))
        return PYION_SDR_ERR;

    // If insert failed, cancel transaction and exit
    //sdr_end_xn(sdr);

    bundleZco = ionCreateZco(ZcoSdrSource, bundleSdr, 0, txInfo->data_size,
                             txInfo->classOfService, 0, ZcoOutbound, state->attendant);
    if (bundleZco == 0) {
        return PYION_IO_ERR;
    }
    ok = bp_send(state->sap, txInfo->destEid, txInfo->reportEid, txInfo->ttl, 
                txInfo->classOfService, txInfo->custodySwitch, txInfo->rrFlags, 
                txInfo->ackReq, txInfo->ancillaryData, bundleZco, &newBundle);

    // If you want custody transfer and have specified a re-transmission timer,
    // then activate it
    if (txInfo->custodySwitch == SourceCustodyRequired && txInfo->retxTimer > 0)
    {
        // Note: The timer starts as soon as bp_memo is called.
        ok = bp_memo(newBundle, txInfo->retxTimer);

        // Handle error in bp_memo
        if (ok < 0)
        {
            return ok;
        }
    }

    // If you have opened this endpoint in detained mode, you need to release the bundle
    if (state->detained)
        bp_release(newBundle);
    return 0;
}