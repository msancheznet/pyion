/**
 * Shim layer for memory module of PyION
 **/
#ifndef BASE_MEM
#define BASE_MEM

#include <zco.h>
#include <sdrxn.h>
#include <psm.h>
#include <memmgr.h>
#include <platform.h>
#include <sdr.h>
#include <ion.h>

typedef struct {
    size_t sp_avail;
    size_t sp_used;
    size_t sp_total;

    // Get amount of data available in large pool [bytes]
    size_t lp_avail;
    size_t lp_used;
    size_t lp_total;

    // Head data, all in [bytes]
    size_t hp_size;
    size_t hp_avail;

} MemInfo;

void base_sdr_usage(Sdr sdr, SdrUsageSummary *sdrUsage);

void base_psm_usage(PsmPartition psm, PsmUsageSummary *psmUsage);
#endif