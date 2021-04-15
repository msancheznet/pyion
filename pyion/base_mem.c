#include <sdr.h>
#include "base_mem.h"


void base_sdr_usage(Sdr sdr, SdrUsageSummary *sdrUsage) {
    return sdr_usage(sdr, sdrUsage);
}

void base_psm_usage(PsmPartition psm, PsmUsageSummary *psmUsage) {
    return psm_usage(psm, psmUsage);
}