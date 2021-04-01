/* ============================================================================
 * Library to query ION's memory state
 *
 * Author: Marc Sanchez Net
 * Date:   04/01/2021
 * Copyright (c) 2021, California Institute of Technology ("Caltech").  
 * U.S. Government sponsorship acknowledged.
 * =========================================================================== */

#ifndef BASEMEM_H
#define BASEMEM_H

#include <platform.h>
#include <sdr.h>
#include <ion.h>
#include <zco.h>
#include <sdrxn.h>
#include <psm.h>
#include <memmgr.h>

// Dumps the state of the SDR
int base_sdr_dump(SdrUsageSummary* sdrUsage);

// Dumps the state of hte PSM
int base_psm_dump(PsmUsageSummary* psmUsage);

#endif
