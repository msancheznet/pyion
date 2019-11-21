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
from unittest.mock import Mock
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
    NO_CUSTODY_REQUESTED    = _bp.NoCustodyRequested
    SOURCE_CUSTODY_OPTIONAL = _bp.SourceCustodyOptional
    SOURCE_CUSTODY_REQUIRED = _bp.SourceCustodyRequired

@unique
class BpPriorityEnum(IntEnum):
    """ BP priority enumeration. See ``help(BpPriorityEnum)`` """
    BP_BULK_PRIORITY      = _bp.BP_BULK_PRIORITY
    BP_STD_PRIORITY       = _bp.BP_STD_PRIORITY
    BP_EXPEDITED_PRIORITY = _bp.BP_EXPEDITED_PRIORITY

@unique
class BpEcsEnumeration(IntEnum):
    """ BP extended class-of-service enumeration. See ``help(BpEcsEnumeration)`` 
    
        - BP_MINIMUM_LATENCY: Forward bundle on all routes
        - BP_BEST_EFFORT: Send using an unreliable convergence layer
        - BP_FLOW_LABEL_PRESENT: Ignore flow label if 0 (deprecated)
        - BP_RELIABLE: Send using a reliable convergence layer
        - BP_RELIABLE_STREAMING: BP_BEST_EFFORT | BP_RELIABLE
    """
    BP_MINIMUM_LATENCY    = _bp.BP_MINIMUM_LATENCY  
    BP_BEST_EFFORT        = _bp.BP_BEST_EFFORT      
    #BP_FLOW_LABEL_PRESENT = _bp.BP_FLOW_LABEL_PRESENT # Not valid in IONv3.7.0
    BP_RELIABLE           = _bp.BP_RELIABLE         
    BP_RELIABLE_STREAMING = _bp.BP_RELIABLE_STREAMING

@unique
class BpReportsEnum(IntEnum):
    """ BP reports enumeration. See ``help(BpReportsEnum)`` """
    BP_NO_RPTS       = 0
    BP_RECEIVED_RPT  = _bp.BP_RECEIVED_RPT
    BP_CUSTODY_RPT   = _bp.BP_CUSTODY_RPT
    BP_FORWARDED_RPT = _bp.BP_FORWARDED_RPT
    BP_DELIVERED_RPT = _bp.BP_DELIVERED_RPT
    BP_DELETED_RPT   = _bp.BP_DELETED_RPT

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
    CFDP_CREATE_FILE  = _cfdp.CfdpCreateFile
    CFDP_DELETE_FILE  = _cfdp.CfdpDeleteFile
    CFDP_RENAME_FILE  = _cfdp.CfdpRenameFile
    CFDP_APPEND_FILE  = _cfdp.CfdpAppendFile
    CFDP_REPLACE_FILE = _cfdp.CfdpReplaceFile
    CFPD_CREATE_DIR   = _cfdp.CfdpCreateDirectory
    CFDP_REMOVE_DIR   = _cfdp.CfdpRemoveDirectory
    CFDP_DENY_FILE    = _cfdp.CfdpDenyFile
    CFDP_DENY_DIR     = _cfdp.CfdpDenyDirectory

@unique
class CfdpEventEnum(IntEnum):
    """ CFDP event types. See ``help(CfdpEventEnum)`` and Section 3.5.6 of CCSDS CFDP """
    CFDP_NO_EVENT                 = _cfdp.CfdpNoEvent
    CFDP_TRANSACTION_IND          = _cfdp.CfdpTransactionInd
    CFDP_EOF_SENT_IND             = _cfdp.CfdpEofSentInd
    CFDP_TRANSACTION_FINISHED_IND = _cfdp.CfdpTransactionFinishedInd
    CFDP_METADATA_RECV_IND        = _cfdp.CfdpMetadataRecvInd
    CFDP_FILE_SEGMENT_IND         = _cfdp.CfdpFileSegmentRecvInd
    CFDP_EOF_RECV_IND             = _cfdp.CfdpEofRecvInd
    CFDP_SUSPENDED_IND            = _cfdp.CfdpSuspendedInd
    CFDP_RESUMED_IND              = _cfdp.CfdpResumedInd
    CFDP_REPORT_IND               = _cfdp.CfdpReportInd
    CFDP_FAULT_IND                = _cfdp.CfdpFaultInd
    CFDP_ABANDONED_IND            = _cfdp.CfdpAbandonedInd
    CFDP_ALL_IND                  = 100

@unique
class CfdpConditionEnum(IntEnum):
    """ CFDP condition types. See ``help(CfdpConditionEnum)`` """
    CFDP_NO_ERROR                 = _cfdp.CfdpNoError
    CFDP_ACK_LIMIT_REACHED        = _cfdp.CfdpAckLimitReached
    CFDP_KEEP_ALIVE_LIMIT_REACHED = _cfdp.CfdpKeepaliveLimitReached
    CFDP_INVALID_TRANS_MODE       = _cfdp.CfdpInvalidTransmissionMode
    CFDP_FILESTORE_REJECT         = _cfdp.CfdpFilestoreRejection
    CFDP_CHECKSUM_FAIL            = _cfdp.CfdpChecksumFailure
    CFDP_FILESIZE_ERROR           = _cfdp.CfdpFileSizeError
    CFDP_NACK_LIMI_REACHED        = _cfdp.CfdpNakLimitReached
    CFDP_INACTIVITY_DETECTED      = _cfdp.CfdpInactivityDetected
    CFDP_INVALID_FILE_STRUCT      = _cfdp.CfdpInvalidFileStructure
    CFDP_CHECK_LIMIT_REACHED      = _cfdp.CfdpCheckLimitReached
    CFDP_SUSPED_REQUESTED         = _cfdp.CfdpSuspendRequested
    CFDP_CANCEL_REQUESTED         = _cfdp.CfdpCancelRequested

@unique
class CfdpFileStatusEnum(IntEnum):
    """ CFDP file status enumeration. See ``help(CfdpFileStatusEnum)`` """
    CFDP_FILE_DISCARDED  = _cfdp.CfdpFileDiscarded
    CFDP_FILE_REJECTED   = _cfdp.CfdpFileRejected
    CFDP_FILE_RETAINED   = _cfdp.CfdpFileRetained
    CFDP_FILE_UNREPORTED = _cfdp.CfdpFileStatusUnreported

@unique
class CfdpDeliverCodeEnum(IntEnum):
    """ CFDP delivery code enumeration. See ``help(CfdpDeliverCodeEnum)`` """
    CFDP_DATA_COMPLETE   = _cfdp.CfdpDataComplete
    CFDP_DATA_INCOMPLETE = _cfdp.CfdpDataIncomplete