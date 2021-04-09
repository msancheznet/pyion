#include <ltp.h>
#include <ion.h>
#include "base_ltp.h"
#include "return_codes.h"
#include "_utils.c"



int base_ltp_attach(void) {
    return ltp_attach();
}

void base_ltp_detach(void) {
    return ltp_detach();
}

int base_ltp_open(unsigned int clientId, LtpSAP **state) {
    // Allocate memory for state and initialize to zeros
    //Replaced memset with calloc for cleaner code
    *state = (LtpSAP*)calloc(1,sizeof(LtpSAP));
    if (*state == NULL) {
        return PYION_MALLOC_ERR;
    }

    // Set memory contents to zeros
    int retStatus = ltp_open(clientId);

    if (retStatus < 0) return retStatus;
     // Fill state information
    (*state)->clientId = clientId;
    (*state)->status = SAP_IDLE;
    return retStatus;

}

int base_ltp_close(LtpSAP *state) {
    ltp_close(state->clientId);

    // Free state memory
    free(state);
    return 0;
}

int base_ltp_interrupt(LtpSAP *state) {
    state->status = SAP_CLOSING;
    ltp_interrupt(state->clientId);
    return 0;
}

int base_ltp_send(LtpSAP *state, LtpTxPayload *msg) {
    char                err_msg[150];
    Sdr sdr;
    int                 ok;
    Object extent;
    Object item = 0;

    sdr = getIonsdr();

    // Start SDR transaction
    if (!sdr_pybegin_xn(sdr)) return PYION_SDR_ERR;

    // Allocate SDR memory
    extent = sdr_insert(sdr, msg->data, (size_t)msg->data_size);
    if (!extent) {
        sdr_cancel_xn(sdr);
        sprintf(err_msg, "SDR memory could not be allocated");
        printf("COULD NOT ALLOCATE SDR MEMORY!!");
        return PYION_SDR_ERR;
    }

    // End SDR transaction
    if (!sdr_pyend_xn(sdr)) return PYION_SDR_ERR;

    // Create ZCO object (not blocking because there is no attendant)
    item = ionCreateZco(ZcoSdrSource, extent, 0, msg->data_size,
                        0, 0, ZcoOutbound, NULL); 

    // Handler error while creating ZCO object
    if (!item || item == (Object)ERROR) {
        sprintf(err_msg, "ZCO object creation failed");
        //zooxzPyErr_SetString(PyExc_RuntimeError, err_msg);
        return PYION_SDR_ERR;
    }

    // Send using LTP protocol. All data is sent as RED LTP by definition.
    // NOTE 1: SessionId is filled by ``ltp_send``, but you do not care about it
    // NOTE 2: In general, ltp_send does not block. However, if you exceed the max
    //         number of export sessions defined in ltprc, then it will.
    ok = ltp_send((uvast)msg->destEngineId, state->clientId, item, LTP_ALL_RED, &(msg->sessionId));
    return ok;
}

int base_ltp_receive_data(LtpSAP *state, LtpRxPayload *payloadObj){
    // Define variables
    char            err_msg[150];
    ZcoReader	    reader;
    Sdr             sdr;
    LtpNoticeType	type;
	LtpSessionId	sessionId;
	unsigned char	endOfBlock;
    unsigned char   reasonCode;
	unsigned int	dataOffset;
	unsigned int	dataLength;
	Object		    data;
    int             receiving_block, notice, do_malloc;
    vast            data_size;

    // Create a pre-allocated buffer for small block sizes. Otherwise, use malloc to
    // get dynamic memory
    

    // Mark as running
    receiving_block = 1;
    state->status = SAP_RUNNING;

    // Process incoming indications
    while ((state->status == SAP_RUNNING) && (receiving_block == 1)) {
        // Get the next LTP notice
        notice = ltp_get_notice(state->clientId, &type, &sessionId, &reasonCode, 
                                &endOfBlock, &dataOffset, &dataLength, &data);

        // Handle error while receiving notices
        if (notice < 0) {
            //Need to set error string receiving notices.
            return PYION_ERR_LTP_NOTICE;
        }
        payloadObj->reasonCode = reasonCode;
        // Handle different notice types
        switch (type) {
            case LtpExportSessionComplete:      // Transmit success
                // Do not mark reception of block complete here. This should be done by the 
                // LtpRecvRedPart case.
                break;
            case LtpImportSessionCanceled:      // Cancelled sessions. No data has been received yet.
                // Release any data and throw exception
                ltp_release_data(data);      
                return PYION_ERR_LTP_IMPORT;
            case LtpExportSessionCanceled:      // Transmit failure. Some data might have been received already.
                // Release any data and throw exception
                ltp_release_data(data);
                return PYION_ERR_LTP_EXPORT;
            case LtpRecvRedPart:
                // If this is not the end of the block, you are dealing with a block that is
                // partially green, partially red. This is not allowed.
                if (!endOfBlock) {
                    ltp_release_data(data);
                    return PYION_ERR_LTP_GREEN;
                }
                
                // Mark that reception of block has ended
                receiving_block = 0;
                break;
            case LtpRecvGreenSegment:
                // Release any data and throw exception
                ltp_release_data(data);
                return PYION_ERR_LTP_RED;
            default:
                break;
        }

        // Make sure other tasks have a chance to run
        sm_TaskYield();
    }

    // If you exited because of closing, throw error
    if (state->status == SAP_CLOSING) {
        return PYION_ERR_LTP_RECEPTION_CLOSED;
    }

    // If no block received by now and you are not closing, error.
    if (receiving_block == -1) {
        return PYION_ERR_LTP_BLOCK_NOT_DELIVERED;
    }

    // Get ION SDR
    sdr = getIonsdr();

    // Get content data size
    if (!sdr_pybegin_xn(sdr)) return PYION_SDR_ERR;
    data_size = zco_source_data_length(sdr, data);
    sdr_exit_xn(sdr);

    // Check if we need to allocate memory dynamically
    do_malloc = 1;

    // Allocate memory for payload
    payloadObj->payload =  (char*)malloc(data_size);

    // Prepare to receive next block
    zco_start_receiving(data, &reader);

    // Get bundle data
    if (!sdr_pybegin_xn(sdr)) return PYION_SDR_ERR;
    payloadObj->len = zco_receive_source(sdr, &reader, data_size, payloadObj->payload);
    if (!sdr_pyend_xn(sdr)) return PYION_SDR_ERR;


    // Handle error while getting the payload
    if (payloadObj->len < 0) {
        
        if (do_malloc) free(payloadObj->payload);
        // Clean up tasks
        
        return PYION_ERR_LTP_EXTRACT;
    }

    // Build return object
   

    // Release LTP object now that you are done with it.
    ltp_release_data(data);

    // Deallocate memory
   

    return 0;
}
