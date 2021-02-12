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

int base_bp_attach() {
    int result = bp_attach();

    return result;
}


int base_bp_detach() {
    bp_detach();
    return 0;
}


static void base_close_endpoint(BpSapState *state) {
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

static int base_bp_interrupt(BpSapState *state) {
    bp_interrupt(state->sap);
     // Pause the attendant
    if (state->attendant) ionPauseAttendant(state->attendant);
    return 0;
}


static int help_receive_data(BpSapState *state, BpDelivery *dlv){
    // Define variables
    int data_size, len, rx_ret, do_malloc;
    Sdr sdr;
    ZcoReader reader;

    // Define variables to store the bundle payload. If payload size is less than
    // MAX_PREALLOC_BUFFER, then use preallocated buffer to save time. Otherwise,
    // call malloc to allocate as much memory as you need.
    char prealloc_payload[MAX_PREALLOC_BUFFER];
    char *payload;

    // Get ION's SDR
    sdr = bp_get_sdr();

    while (state->status == EID_RUNNING) {
        // Receive the next bundle. This is a blocking call. Therefore, release the GIL
        rx_ret = bp_receive(state->sap, dlv, BP_BLOCKING);
                                    // Acquire the GIL

        // Check if error while receiving a bundle
        if ((rx_ret < 0) && (state->status == EID_RUNNING)) {
            return PYION_IO_ERR;
            //pyion_SetExc(PyExc_IOError, "Error receiving bundle through endpoint (err code=%d).", rx_ret);
            //return NULL;
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
        //pyion_SetExc(PyExc_InterruptedError, "BP reception interrupted.");
        return PYION_INTERRUPTED_ERR;
        //return NULL;
    }

    // If you exited because of closing
    if (state->status == EID_CLOSING) {
        return  PYION_CONN_ABORTED_ERR;
      //  pyion_SetExc(PyExc_ConnectionAbortedError, "BP reception closed.");
       // return NULL;
    }

    // If endpoint was stopped, finish
    if (dlv->result == BpEndpointStopped) {
        return  PYION_CONN_ABORTED_ERR;

        //pyion_SetExc(PyExc_ConnectionAbortedError, "BP endpoint was stopped.");
        //return NULL;
    }

    // If bundle does not have the payload, raise IOError
    if (dlv->result != BpPayloadPresent) {
        return PYION_IO_ERR;
        //pyion_SetExc(PyExc_IOError, "Bundle received without payload.");
        //return NULL;
    }

    // Get content data size
    if (!sdr_pybegin_xn(sdr)) return NULL;
    data_size = zco_source_data_length(sdr, dlv->adu);
    sdr_pyexit_xn(sdr);

    // Check if we need to allocate memory dynamically
    do_malloc = (data_size > MAX_PREALLOC_BUFFER);

    // Allocate memory if necessary
    payload = do_malloc ? (char *)malloc(data_size) : prealloc_payload;

    // Initialize reader
    zco_start_receiving(dlv->adu, &reader);

    // Get bundle data
    if (!sdr_pybegin_xn(sdr)) return NULL;
    len = zco_receive_source(sdr, &reader, data_size, payload);

    // Handle error while getting the payload
    if (sdr_end_xn(sdr) < 0 || len < 0) {
        //pyion_SetExc(PyExc_IOError, "Error extracting payload from bundle.");

        // Clean up tasks
        if (do_malloc) free(payload);
        return PYION_IO_ERR;
        //return NULL;
    }

    // Build return object
    PyObject *ret = Py_BuildValue("y#", payload, len);

    // If you allocated memory for this payload, free it here
    if (do_malloc) free(payload);

    return ret;
}


static int base_bp_receive_data(BpSapState *state) {
    BpDelivery dlv;

    int status = help_receive_data(state, &dlv);

    // Close if necessary. Otherwise set to IDLE
    if (state->status == EID_CLOSING) {
        base_close_endpoint(state);
    } else {
        state->status = EID_IDLE;
    }




}


int base_bp_open(BpSapState *state, int mem_ctrl) {
        // Define variables
    char *ownEid;
    int detained, ok;
    //state = (BpSapState*)malloc(sizeof(BpSapState));
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
int base_bp_send(char *destEid, char *reportEid, int ttl, int classOfService,
                 int custodySwitch, int rrFlags, int ackReq, unsigned int retxTimer,
                 BpAncillaryData *ancillaryData, int data_size)
{

    Object newBundle;
    BpSapState *state = NULL;
    const char *data = NULL;
    Sdr sdr = NULL;
    Object bundleZco;
    Object bundleSdr;
    int ok;

    // Initialize variables
    sdr = bp_get_sdr();
    printf("THIS IS SOMETHING THAT I ADDED!");

    // Insert data to SDR
    if (!sdr_pybegin_xn(sdr))
        return 1;
    bundleSdr = sdr_insert(sdr, data, (size_t)data_size);
    if (!sdr_pyend_xn(sdr))
        return 1;

    // If insert failed, cancel transaction and exit

    bundleZco = ionCreateZco(ZcoSdrSource, bundleSdr, 0, data_size,
                             classOfService, 0, ZcoOutbound, state->attendant);
    if (bundleZco == 0) {
        return 2;
    }
    ok = bp_send(state->sap, destEid, reportEid, ttl, classOfService, custodySwitch,
                 rrFlags, ackReq, ancillaryData, bundleZco, &newBundle);

    // If you want custody transfer and have specified a re-transmission timer,
    // then activate it
    if (custodySwitch == SourceCustodyRequired && retxTimer > 0)
    {
        // Note: The timer starts as soon as bp_memo is called.
        ok = bp_memo(newBundle, retxTimer);

        // Handle error in bp_memo
        if (ok < 0)
        {
            return 3;
        }
    }

    // If you have opened this endpoint in detained mode, you need to release the bundle
    if (state->detained)
        bp_release(newBundle);
    return 0;
}