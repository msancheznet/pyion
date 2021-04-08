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