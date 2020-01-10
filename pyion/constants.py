""" 
# ===========================================================================
# This file constains all the constants used in pyion
#
# Author: Marc Sanchez Net
# Date:   04/17/2019
# Copyright (c) 2019, California Institute of Technology ("Caltech").  
# U.S. Government sponsorship acknowledged.
# ===========================================================================
"""

# Generic imports
from enum import IntEnum, unique
from mock import Mock
from warnings import warn

# Import C Extensions
try:
    import _bp
    import _cfdp
except ImportError:
    warn('_bp, _cfdp extensions not available. Using mock instead.')
    _bp, _cfdp = Mock(), Mock()

# Define all methods/vars exposed at pyion
__all__ = [
    'BpCustodyEnum', 
    'BpPriorityEnum',
    'BpEcsEnumeration',
    'BpReportsEnum',
    'BpAckReqEnum',
    'CfdpMode',
    'CfdpClosure',
    'CfdpMetadataEnum',
    'CfdpFileStoreEnum',
    'CfdpEventEnum',
    'CfdpConditionEnum',
    'CfdpFileStatusEnum',
    'CfdpDeliverCodeEnum'
]

# ============================================================================
# === BUNDLE PROTOCOL
# ============================================================================

@unique
class BpCustodyEnum(IntEnum):
    """ BP custody switch enumeration. See ``help(CustodyEnum)`` """
    NO_CUSTODY_REQUESTED    = 0
    SOURCE_CUSTODY_OPTIONAL = 0
    SOURCE_CUSTODY_REQUIRED = 0

@unique
class BpPriorityEnum(IntEnum):
    """ BP priority enumeration. See ``help(BpPriorityEnum)`` """
    BP_BULK_PRIORITY      = 0
    BP_STD_PRIORITY       = 0
    BP_EXPEDITED_PRIORITY = 0

@unique
class BpEcsEnumeration(IntEnum):
    """ BP extended class-of-service enumeration. See ``help(BpEcsEnumeration)`` 
    
        - BP_MINIMUM_LATENCY: Forward bundle on all routes
        - BP_BEST_EFFORT: Send using an unreliable convergence layer
        - BP_FLOW_LABEL_PRESENT: Ignore flow label if 0
        - BP_RELIABLE: Send using a reliable convergence layer
        - BP_RELIABLE_STREAMING: BP_BEST_EFFORT | BP_RELIABLE
    """
    BP_MINIMUM_LATENCY    = 0
    BP_BEST_EFFORT        = 0
    BP_FLOW_LABEL_PRESENT = 0
    BP_RELIABLE           = 0      
    BP_RELIABLE_STREAMING = 0

@unique
class BpReportsEnum(IntEnum):
    """ BP reports enumeration. See ``help(BpReportsEnum)`` """
    BP_NO_RPTS       = 0
    BP_RECEIVED_RPT  = 0
    BP_CUSTODY_RPT   = 0
    BP_FORWARDED_RPT = 0
    BP_DELIVERED_RPT = 0
    BP_DELETED_RPT   = 0

@unique
class BpAckReqEnum(IntEnum):
    """ BP application acknowledgement enumeration. See ``help(BpAckReqEnum)`` """
    BP_ACK_REQ    = True
    BP_NO_ACK_REQ = False

# ============================================================================
# === CFDP PROTOCOL
# ============================================================================

@unique
class CfdpMode(IntEnum):
    """ CFDP mode enumeration. See ``help(CfdpMode)`` 
    
        - CFDP_CL_RELIABLE: Reliability provided by the convergence layer
        - CFDP_UNRELIABLE: No reliability provided
        - CFDP_BP_RELIABLE: Reliability provided by BP custody transfer
    """
    CFDP_CL_RELIABLE = 0    # Reliability provided by convergence layer
    CFDP_UNRELIABLE  = 1    # No reliability provided
    CFDP_BP_RELIABLE = 2    # Reliability provided by custody transfer

@unique
class CfdpClosure(IntEnum):
    """ Length of time following transmission of the EOF PDU within which a responding
        Transaction Finish PDU is expected. If no Finish PDU is requested, this parameter value should be
        zero.

        .. Tip:: If you need a number of seconds not specified as an option in this enumeration,
                 simply pass it directly.
    """
    CFDP_NO_CLOSURE = 0     # No closure.
    CFDP_1_SEC      = 1
    CFDP_1_MIN      = 60
    CFDP_5_MIN      = 300
    CFDP_1_HR       = 3600

@unique
class CfdpMetadataEnum(IntEnum):
    """ CFDP segment metadata options. See ``help(CfdpMetadataEnum)``

        - CFDP_NO_SEG_METADATA: No metadata added
        - CFDP_SEG_METADATA: Adds current time as string to all PDUs
    """
    CFDP_NO_SEG_METADATA = 0
    CFDP_SEG_METADATA    = 1

@unique
class CfdpFileStoreEnum(IntEnum):
    """ CFDP file store actions. See ``help(CfdpFileStoreEnum)`` """
    CFDP_CREATE_FILE  = 0
    CFDP_DELETE_FILE  = 0
    CFDP_RENAME_FILE  = 0
    CFDP_APPEND_FILE  = 0
    CFDP_REPLACE_FILE = 0
    CFPD_CREATE_DIR   = 0
    CFDP_REMOVE_DIR   = 0
    CFDP_DENY_FILE    = 0
    CFDP_DENY_DIR     = 0

@unique
class CfdpEventEnum(IntEnum):
    """ CFDP event types. See ``help(CfdpEventEnum)`` and Section 3.5.6 of CCSDS CFDP """
    CFDP_NO_EVENT                 = 0
    CFDP_TRANSACTION_IND          = 0
    CFDP_EOF_SENT_IND             = 0
    CFDP_TRANSACTION_FINISHED_IND = 0
    CFDP_METADATA_RECV_IND        = 0
    CFDP_FILE_SEGMENT_IND         = 0
    CFDP_EOF_RECV_IND             = 0
    CFDP_SUSPENDED_IND            = 0
    CFDP_RESUMED_IND              = 0
    CFDP_REPORT_IND               = 0
    CFDP_FAULT_IND                = 0
    CFDP_ABANDONED_IND            = 0
    CFDP_ALL_IND                  = 100

@unique
class CfdpConditionEnum(IntEnum):
    """ CFDP condition types. See ``help(CfdpConditionEnum)`` """
    CFDP_NO_ERROR                 = 0
    CFDP_ACK_LIMIT_REACHED        = 0
    CFDP_KEEP_ALIVE_LIMIT_REACHED = 0
    CFDP_INVALID_TRANS_MODE       = 0
    CFDP_FILESTORE_REJECT         = 0
    CFDP_CHECKSUM_FAIL            = 0
    CFDP_FILESIZE_ERROR           = 0
    CFDP_NACK_LIMI_REACHED        = 0
    CFDP_INACTIVITY_DETECTED      = 0
    CFDP_INVALID_FILE_STRUCT      = 0
    CFDP_CHECK_LIMIT_REACHED      = 0
    CFDP_SUSPED_REQUESTED         = 0
    CFDP_CANCEL_REQUESTED         = 0

@unique
class CfdpFileStatusEnum(IntEnum):
    """ CFDP file status enumeration. See ``help(CfdpFileStatusEnum)`` """
    CFDP_FILE_DISCARDED  = 0
    CFDP_FILE_REJECTED   = 0
    CFDP_FILE_RETAINED   = 0
    CFDP_FILE_UNREPORTED = 0

@unique
class CfdpDeliverCodeEnum(IntEnum):
    """ CFDP delivery code enumeration. See ``help(CfdpDeliverCodeEnum)`` """
    CFDP_DATA_COMPLETE   = 0
    CFDP_DATA_INCOMPLETE = 0