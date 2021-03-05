#include <ltp.h>
#include "base_ltp.h"
#include "return_codes.h"



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