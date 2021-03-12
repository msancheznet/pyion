#include <ltp.h>
#include <ion.h>
#include "base_ltp.h"
#include "return_codes.h"
#include "_utils.c"



int base_ltp_attach(void) {
    return ltp_attach();
}

int base_ltp_detach(void) {
    ltp_detach();
    return 0;
}

int base_ltp_open(unsigned int clientId, LtpSAP *state) {
    char err_msg[150];
    // Allocate memory for state and initialize to zeros
    state = (LtpSAP*)malloc(sizeof(LtpSAP));
    if (state == NULL) {
        sprintf(err_msg, "Cannot malloc for LTP state.");
        return PYION_MALLOC_ERR;
    }

    // Set memory contents to zeros
    memset((char *)state, 0, sizeof(LtpSAP));
    int retStatus = ltp_open(clientId);

    if (retStatus < 0) return retStatus;
     // Fill state information
    state->clientId = clientId;
    state->status = SAP_IDLE;
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