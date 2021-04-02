/* ============================================================================
 * Utility macros for base_bp
 *
 * Author: Marc Sanchez Net
 * Date:   03/22/2021
 * Copyright (c) 2021, California Institute of Technology ("Caltech").  
 * U.S. Government sponsorship acknowledged.
 * =========================================================================== */

#ifndef MACROS_H_
#define MACROS_H_

/* ============================================================================
 * === RETURN CODES
 * ============================================================================ */

// Generic OK/ERR codes
#define PYION_OK 0
#define PYION_ERR -10000

// Specific error codes
#define PYION_SDR_ERR -10001
#define PYION_PSM_ERR -10007
#define PYION_IO_ERR -10002
#define PYION_INTERRUPTED_ERR -10003
#define PYION_CONN_ABORTED_ERR -10004
#define PYION_MALLOC_ERR  -10005
#define PYION_ZCO_ERR -10006


/* ============================================================================
 * === SDR MACROS
 * ============================================================================ */

#define SDR_BEGIN_XN \
    if (!sdr_begin_xn(sdr)) { \
        return PYION_SDR_ERR; \
    }

#define SDR_END_XN \
    if (sdr_end_xn(sdr) < 0) { \
        return PYION_SDR_ERR; \
    }

#define SDR_EXIT_XN \
    sdr_exit_xn(sdr);

/* ============================================================================
 * === ZCO MACROS
 * ============================================================================ */

#define ZCO_CREATE_ERROR(obj) \
    if (obj == 0 || obj == (Object)ERROR) { \
        return PYION_ZCO_ERR; \
    }

#endif