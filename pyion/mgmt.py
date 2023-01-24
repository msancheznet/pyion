""" 
# ===========================================================================
# Defines management ION functions.
#
# Author: Marc Sanchez Net
# Date:   08/21/2019
# Copyright (c) 2019, California Institute of Technology ("Caltech").  
# U.S. Government sponsorship acknowledged.
# ===========================================================================
"""

# General imports
from unittest.mock import Mock
from warnings import warn

# pyion imports
import pyion.utils as utils

# Import C Extension
import _mgmt

# Define all methods/vars exposed at pyion
_cgr    = ['cgr_list_contacts', 'cgr_list_ranges', 'cgr_add_contact', 
           'cgr_add_range', 'cgr_delete_contact', 'cgr_delete_range',
           'cgr_list_regions']
_bp     = ['bp_endpoint_exists', 'bp_add_endpoint', 'bp_list_endpoints']
_ltp    = ['ltp_span_exists']
_cfdp   = []
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
    _mgmt.bp_add_endpoint(eid, int(discard))

def bp_endpoint_exists(eid):
    """ Check if an endpoint has been defined in ION.

        :param str eid: E.g., ipn:1.1.
        :return bool: True if endpoint is defined in ION
    """
    return _mgmt.bp_endpoint_exists(eid)

def bp_list_endpoints():
    """ List all endpoints defined in ION """
    raise NotImplementedError

# ============================================================================
# === Functions to manage regions
# ============================================================================

def cgr_list_regions():
    """ List all the regions defined in this ION node

        :return List[int]: List of region numbers
    """
    return _mgmt.list_regions()

# ============================================================================
# === Functions to create/delete/modify the contact plan
# ============================================================================

def cgr_list_contacts():
    """ List all contacts in ION's contact plan.
        
        :return List[Dict]: Each dict is a contact defined as:
                            {orig:, dest:, tstart:, tend:, rate:, confidence:, }

        .. Tip:: The data rate is returned in [bits/sec]
    """
    return _mgmt.list_contacts()

def cgr_list_ranges():
    """ List all ranges in ION's contact_plan 

        :return List[Dict]: Each dict is a range defined as:
                            {orig:, dest:, tstart:, tend:, owlt:}
    """
    return _mgmt.list_ranges()

def cgr_add_contact(orig, dest, tstart, tend, rate, region_nbr=1, confidence=1.0, announce=True):
    """ Add a contact to ION's contact plan

        :param int orig: Node number of the contact origin
        :param int dest: Node number of the contact destination
        :param str tstart: Contact start time. Format is ``yyyy/mm/dd-hh:MM:ss`` or ``+0``
        :param str tend: Contact end time. Format is ``yyyy/mm/dd-hh:MM:ss`` or ``+0``
        :param float rate: Contact data rate in [bits/sec].
        :param int region_nbr: Region index. Defaults to 1.
        :param float confidence: Contact confidence. Defaults to 1. 
        :param bool announce: If True, the information of this contact will be multicasted
                              to all nodes in the region. Default is True

        .. Warning:: The contact data rate gets transformed to bytes/sec 
                     and rounded internally
    """
    # If tstart/tend is/are provided in relative format, transform it to absolute
    tstart = utils.rel2abs_time(tstart)
    tend   = utils.rel2abs_time(tend)

    # Add the contact
    _mgmt.add_contact(region_nbr, orig, dest, tstart, tend, int(rate/8), confidence, int(announce))

def cgr_add_range(orig, dest, tstart, tend, owlt=0.0, announce=True):
    """ Add a range to ION's contact plan

        :param int orig: Node number of the contact origin
        :param int dest: Node number of the contact destination
        :param str tstart: Contact start time. Format is ``yyyy/mm/dd-hh:MM:ss``
        :param str tend: Contact end time. Format is ``yyyy/mm/dd-hh:MM:ss``
        :param float owlt: Range in light seconds.
        :param bool announce: If True, the information of this contact will be multicasted
                              to all nodes in the region. Default is True

        .. Warning:: The owlt gets rounded internally
    """
    # If tstart/tend is/are provided in relative format, transform it to absolute
    tstart = utils.rel2abs_time(tstart)
    tend   = utils.rel2abs_time(tend)

    # Add the range
    _mgmt.add_range(orig, dest, tstart, tend, int(owlt), int(announce))

def cgr_delete_contact(orig, dest, tstart=None, region_nbr=1, announce=True):
    """ Delete a contact from ION's contact plan

        :param int orig: Node number of the contact origin
        :param int dest: Node number of the contact destination. 
        :param str tstart: Contact start time. If None, all contacts between orig and
                           dest are deleted. Format is ``yyyy/mm/dd-hh:MM:ss``
        :param int region_nbr: Region number for this node. Defaults to 1                           
        :param bool announce: If True, the information of this contact will be multicasted
                              to all nodes in the region. Default is True                           
    """
    # If tstart/tend is/are provided in relative format, transform it to absolute
    if tstart: tstart = utils.rel2abs_time(tstart)

    # Delete the contact
    _mgmt.delete_contact(region_nbr, orig, dest, tstart, int(announce))

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
    _mgmt.delete_range(orig, dest, tstart)

# ============================================================================
# === LTP-related functions
# ============================================================================

def ltp_span_exists(engine_nbr):
    """ Determines if an LTP span is defined within ION.

        :param int engine_nbr: Peer engine number.
        :return bool: True if span exists.
    """
    return _mgmt.ltp_span_exists(engine_nbr)