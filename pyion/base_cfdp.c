#include <cfdp.h>
#include <bputa.h>

#include "base_cfdp.h"
/**
 * Worker file to handle pure C function
 * interaction between PyION and ION.
 * 
 */


/**
 * 
 * cfdp param initialiation function. Compress the destination eid 
 * from a particular uvast. Then, apply the appropriate 
 * anciallyData flags based off of our given criticality.
 */
int base_cfdp_open(CfdpReqParms *params, uvast from, int criticality) {
    cfdp_compress_number(&(params->destinationEntityNbr), from);
    if (criticality == 1)
		params->utParms.ancillaryData.flags |= BP_MINIMUM_LATENCY;
	else
		params->utParms.ancillaryData.flags &= (~BP_MINIMUM_LATENCY);

    return 0;
}

int base_cfdp_attach() {
    return cfdp_attach();
}

void base_cfdp_detach() {
    return cfdp_detach();
}

int base_cfdp_add_usrmsg(MetadataList list, unsigned char *text){
    return cfdp_add_usrmsg(list, text, strlen(text)+1);
}

MetadataList base_cfdp_create_usrmsg_list(void) {
    return cfdp_create_usrmsg_list();
}

MetadataList base_cfdp_create_fsreq_list(void) {
    return cfdp_create_fsreq_list();
}

int base_cfdp_add_fsreq(MetadataList list, CfdpAction action, char *firstFileName, char *secondFileName) {
    return cfdp_add_fsreq(list, action, firstFileName, secondFileName);
}


void setParams(CfdpReqParms *params, char *sourceFile, char *destFile, 
                      int segMetadata, int closureLat, long int mode) {
    // Fill in basic parameters
    params->sourceFileName = sourceFile;
    params->destFileName   = destFile;
    params->segMetadataFn = (segMetadata==0) ? NULL : noteSegmentTime;
    params->closureLatency = closureLat;

    // mode = 1: Select unreliable CFDP mode
    if (mode & 0x01) {
        params->utParms.ancillaryData.flags |= BP_BEST_EFFORT;
        return;
    }

    // mode = 2: Select native BP reliability
    if (mode & 0x02) {
        params->utParms.custodySwitch = SourceCustodyRequired;
        return;
    }

    // Mode = 3: Select CL reliability
    params->utParms.ancillaryData.flags &= (~BP_BEST_EFFORT);
    params->utParms.custodySwitch = NoCustodyRequested;
}

/* ============================================================================
 * === Send/Request Functions (and helpers)
 * ============================================================================ */

int	noteSegmentTime(uvast fileOffset, unsigned int recordOffset,
			unsigned int length, int sourceFileFd, char *buffer) {
	writeTimestampLocal(getCtime(), buffer);
	return strlen(buffer) + 1;
}
