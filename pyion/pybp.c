/**
 * Worker file to handle pure C function
 * interaction between PyION and ION.
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <bp.h>

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