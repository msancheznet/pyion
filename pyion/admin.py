""" 
# ===========================================================================
# Defines administrative ION functions.
#
# Author: Marc Sanchez Net
# Date:   08/21/2019
# Copyright (c) 2019, California Institute of Technology ("Caltech").  
# U.S. Government sponsorship acknowledged.
# ===========================================================================
"""

# General imports
from mock import Mock
from warnings import warn

# pyion imports
import pyion.utils as utils

# Import C Extension
try:
	import _admin
except ImportError:
	warn('_admin extension not available. Using mock instead.')
	_admin = Mock()

# Define all methods/vars exposed at pyion
_cgr    = ['cgr_list_contacts', 'cgr_list_ranges', 'cgr_add_contact', 
           'cgr_add_range', 'cgr_delete_contact', 'cgr_delete_range']
_bp     = ['bp_endpoint_exists', 'bp_add_endpoint', 'bp_list_endpoints']
_ltp    = ['ltp_span_exists', 'ltp_update_span', 'ltp_info_span']
_cfdp   = ['cfdp_update_pdu_size']
__all__ = _cgr + _bp + _ltp + _cfdp

# ============================================================================
# === Functions to add endpoints
# ============================================================================

def bp_add_endpoint(eid, discard=True):
    """ Define a new endpoint in ION

        :param str eid: E.g., ipn:1.1. (ipn:x.0 not valid)
        :param bool discard: If True, bundles arriving to and endpoint that 
                             has not application attached to it will be discarded.
                             Otherwise they will be enqueued for later delivery.
    """
    # If this endpoint is already defined, skip
    if bp_endpoint_exists(eid): return

    # Add the new endpoint.
    _admin.bp_add_endpoint(eid, int(discard))

def bp_endpoint_exists(eid):
    """ Check if an endpoint has been defined in ION.

        :param str eid: E.g., ipn:1.1.
        :return bool: True if endpoint is defined in ION
    """
    return _admin.bp_endpoint_exists(eid)

def bp_list_endpoints():
    """ List all endpoints defined in ION """
    raise NotImplementedError

# ============================================================================
# === Functions to create/delete/modify the contact plan
# ============================================================================

def cgr_list_contacts():
    """ List all contacts in ION's contact plan.
        
        :return List[Dict]: Each dict is a contact defined as:
                            {orig:, dest:, tstart:, tend:, rate:, confidence:, }

        .. Tip:: The data rate is returned in [bits/sec]
    """
    return _admin.list_contacts()

def cgr_list_ranges():
    """ List all ranges in ION's contact_plan 

        :return List[Dict]: Each dict is a range defined as:
                            {orig:, dest:, tstart:, tend:, owlt:}
    """
    return _admin.list_ranges()

def cgr_add_contact(orig, dest, tstart, tend, rate, confidence=1.0):
    """ Add a contact to ION's contact plan

        :param int orig: Node number of the contact origin
        :param int dest: Node number of the contact destination
        :param str tstart: Contact start time. Format is ``yyyy/mm/dd-hh:MM:ss``
        :param str tend: Contact end time. Format is ``yyyy/mm/dd-hh:MM:ss``
        :param float rate: Contact data rate in [bits/sec].
        :param float confidence: Contact confidence. Defaults to 1. 

        .. Warning:: The contact data rate gets transformed to bytes/sec 
                     and rounded internally
    """
    # If tstart/tend is/are provided in relative format, transform it to absolute
    tstart = utils.rel2abs_time(tstart)
    tend   = utils.rel2abs_time(tend)

    # Add the contact
    _admin.add_contact(orig, dest, tstart, tend, int(rate/8), confidence)

def cgr_add_range(orig, dest, tstart, tend, owlt=0.0):
    """ Add a range to ION's contact plan

        :param int orig: Node number of the contact origin
        :param int dest: Node number of the contact destination
        :param str tstart: Contact start time. Format is ``yyyy/mm/dd-hh:MM:ss``
        :param str tend: Contact end time. Format is ``yyyy/mm/dd-hh:MM:ss``
        :param float owlt: Range in light seconds.

        .. Warning:: The owlt gets rounded internally
    """
    # If tstart/tend is/are provided in relative format, transform it to absolute
    tstart = utils.rel2abs_time(tstart)
    tend   = utils.rel2abs_time(tend)

    # Add the range
    _admin.add_range(orig, dest, tstart, tend, int(owlt))

def cgr_delete_contact(orig, dest, tstart=None):
    """ Delete a contact from ION's contact plan

        :param int orig: Node number of the contact origin
        :param int dest: Node number of the contact destination. 
        :param str tstart: Contact start time. If None, all contacts between orig and
                           dest are deleted. Format is ``yyyy/mm/dd-hh:MM:ss``
    """
    # If tstart/tend is/are provided in relative format, transform it to absolute
    if tstart: tstart = utils.rel2abs_time(tstart)

    # Delete the contact
    _admin.delete_contact(orig, dest, tstart)

def cgr_delete_range(orig, dest, tstart=None):
    """ Delete a range from ION's contact plan

        :param int orig: Node number of the contact origin
        :param int dest: Node number of the contact destination
        :param str tstart: Range start time. If None, all ranges between orig and dest
                           are deleted. Format is ``yyyy/mm/dd-hh:MM:ss``
    """
    # If tstart/tend is/are provided in relative format, transform it to absolute
    if tstart: tstart = utils.rel2abs_time(tstart)

    # Delete the range
    _admin.delete_range(orig, dest, tstart)

# ============================================================================
# === LTP-related functions
# ============================================================================

def ltp_span_exists(engine_nbr):
    """ Determines if an LTP span is defined within ION.

        :param int engine_nbr: Peer engine number.
        :return bool: True if span exists.
    """
    return _admin.ltp_span_exists(engine_nbr)

def ltp_info_span(engine_nbr=None, force_list=False):
    """ Get information about one or multiple LTP spans

        :param int engine_nbr: Peer node number for which you want the span info.
                               Defaults to None, which will make this function
                               return a list of all defined spans
        :param bool force_list: If True, this function returns a list. If false,
                                and only one span is found, then it is depacked from
                                the list
        :return Or[List[Dict], Dict]: The dictionary contains the span information.
    """
    # For now, C call results in a segmentation fault
    raise NotImplementedError

    # The C function expects -1 as None
    if engine_nbr is None: engine_nbr = -1

    # Get the information for the spans
    spans = _admin.ltp_info_span(engine_nbr)

    # If you only have one entry and do not want a list, return first entry
    if not force_list and len(spans) == 1:
        spans = spans[0]

    return spans

def ltp_update_span(engine_nbr, max_segment_size, ip_addr, udp_port, udp_rate=0, max_export_sessions=1,
                    max_import_sessions=1, agg_size_limit=1, agg_time_limit=1, q_lat=1, purge=False):
    """ Update a span's configuration parameters

        :param int engine_nbr: Peer engine number.
        :param int max_segment_size: Max size of an LTP data segment in [bytes].
        :param str ip_addr: IP address of the peer node (for udplso command).
        :param int upd_port: Port number where the peer's udplsi is listening to.
        :param int udp_rate: Rate in [bytes/sec] at which data should be pushed from LTP to UDP.
                             This is here to ensure that you do not overflow the UPD buffers.
                             Defaults to 0, i.e., no rate control.
        :param int max_export_sessions: Max number of LTP blocks that can be concurrently transmitted
                                        (each block is equal to a session). Defaults to 1.
        :param int max_import_sessions: The ``max_export_session`` specified in the peer engine.
                                        Defaults to 1.
        :param int agg_size_limit: Number of bundles that are agreggated into an LTP block.
                                   Defaults to 1.
        :param int agg_time_limit: Amount of time (in sec) LTP waits before it start processing a given block.
                                   I.e., if the block does not have enough bundles as specified in
                                   ``agg_size_limit`` and this timer expires, it will be processed.
                                   Defaults to 1 second.
        :param int q_lat: Estimated number of seconds that are expected to lapse between reception of a 
                          segment at this node and transmission of an acknowledgement segment.
                          Defaults to 1 second.
        :param bool purge: See ION manual. Defaults to False.
    """
    # Check if the LTP span exists.
    if not ltp_span_exists(engine_nbr):
        raise KeyError('An LTP span to peer {} is not defined.'.format(engine_nbr))

    # Create the lso command
    lso_cmd = 'udplso {}:{} {}'.format(ip_addr, udp_port, udp_rate)

    # Update the ltp span
    _admin.ltp_update_span(engine_nbr, max_export_sessions, max_import_sessions, max_segment_size,
                            agg_size_limit, agg_time_limit, lso_cmd, q_lat, int(purge))

# ============================================================================
# === CFDP-related functions
# ============================================================================

def cfdp_update_pdu_size(pdu_size):
    """ Update the PDU size of the local CFDP engine

        :param int: Max PDU size in [bytes]
    """
    _admin.cfdp_update_pdu_size(pdu_size)