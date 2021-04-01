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


typedef struct
{
    char *payload;
    int len;
    int do_malloc;
} LtpRxPayload;




typedef struct
{
    unsigned long long destEngineId;
    char *data;
    int data_size;
    LtpSessionId   sessionId;

} LtpTxPayload;



/*
Attaches to ltp endpoint.
*/
int base_ltp_attach(void);

void base_ltp_detach(void);

int base_ltp_open(unsigned int clientId, LtpSAP **state);

int base_ltp_close(LtpSAP *state);

int base_ltp_interrupt(LtpSAP *state);

int base_ltp_send(LtpSAP *state, LtpTxPayload *txInfo);


#endif