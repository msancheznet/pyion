#ifndef BASE_LTP_H
#define BASE_LTP_H

#include <ltp.h>
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



/*
Attaches to ltp endpoint.
*/
int base_ltp_attach(void);

int base_ltp_detach(void);

int base_ltp_open(unsigned int clientId, LtpSAP *state);

int base_ltp_close(LtpSAP *state);


#endif