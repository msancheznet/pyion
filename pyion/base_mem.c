/* ============================================================================
 * Library to query ION's memory state
 *
 * Author: Marc Sanchez Net
 * Date:   04/01/2021
 * Copyright (c) 2021, California Institute of Technology ("Caltech").  
 * U.S. Government sponsorship acknowledged.
 * =========================================================================== */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "base_mem.h"
#include "_utils.c"
#include "macros.h"

/* ============================================================================
 * === SDR
 * ============================================================================ */

int base_sdr_dump(SdrUsageSummary* sdrUsage) {
    // Attach to SDR 
    Sdr sdr = getIonsdr();
    if (sdr == NULL) return PYION_SDR_ERR;

    // Get the state of the SDR
    SDR_BEGIN_XN
    sdr_usage(sdr, sdrUsage);
    SDR_END_XN

    return PYION_OK;
}

/* ============================================================================
 * === PSM
 * ============================================================================ */

int base_psm_dump(PsmUsageSummary* psmUsage) {
    // Attach to PSM 
    PsmPartition psm = getIonwm();
    if (psm == NULL) return PYION_PSM_ERR;

    // Get the state of the SDR
    psm_usage(psm, psmUsage);

    return PYION_OK;
}