""" 
# ===========================================================================
# Interface between the ION and Python. Provides ``Proxy`` object to connect
# to ION.
#
# Author: Marc Sanchez Net
# Date:   04/17/2019
# Copyright (c) 2019, California Institute of Technology ("Caltech").  
# U.S. Government sponsorship acknowledged.
# ===========================================================================
"""

import os
import signal
from pathlib import Path
from time import sleep
# General imports
from unittest.mock import Mock
from warnings import warn

# Module imports
import pyion
import pyion.bp as bp
import pyion.cfdp as cfdp
import pyion.constants as cst
import pyion.ltp as ltp
import pyion.mem as mem
import pyion.utils as utils

# Import C Extensions
import _bp
import _cfdp
import _ltp

# Define all methods/vars exposed at pyion
__all__ = ['get_bp_proxy', 'get_cfdp_proxy', 'get_ltp_proxy', 'get_sdr_proxy',
           'get_psm_proxy']

# ============================================================================
# === Singleton of proxies
# ============================================================================

# Map of proxies to BP defined as {node_nbr: BpProxy}
_bp_proxies = {}

# Map of proxies to CFDP defined as {peer_entity_nbr: CfdpProxy}
_cfdp_proxies = {}

# Map of proxies to LTP defined as {client_id: LtpProxy}
_ltp_proxies = {}

# Map of proxies to the SDR defined as {(node_nbr, sdr_name): SdrProxy}
_sdr_proxies = {}

# Map of proxies to the PSM defined as {(node_nbr, wm_key): PsmProxy}
_psm_proxies = {}

def get_bp_proxy(node_nbr):
    """ Returns a BpProxy for a given node number. If it already exists, it
        gives you the already instantiated copy.

        :param:  Node number
        :return: BpProxy object
    """
    global _bp_proxies
    proxy = utils._register_proxy(_bp_proxies, str(node_nbr), BpProxy, node_nbr)
    proxy.bp_attach()
    return proxy

def get_cfdp_proxy(peer_entity_nbr):
    """ Returns a CfdpProxy for a given peer entity number. If it already exists, it
        gives you the already instantiated copy.

        :param:  Peer entity number
        :return: CfdpProxy object
    """
    global _cfdp_proxies
    proxy = utils._register_proxy(_cfdp_proxies, str(peer_entity_nbr), CfdpProxy, peer_entity_nbr)
    proxy.cfdp_attach()
    return proxy

def get_ltp_proxy(client_id):
    """ Returns an LtpProxy for a given client application. If it already exists, it
        gives you the already instantiated copy.

        :param:  Client id (number)
        :return: LtpProxy object
    """
    global _ltp_proxies
    proxy = utils._register_proxy(_ltp_proxies, str(client_id), LtpProxy, client_id)
    proxy.ltp_attach()
    return proxy

def get_sdr_proxy(node_nbr):
    """ Return an SdrProxy for a given client application. If it already exists, it
        gives you th ealready instantiated copy

        :param: Node number
        :return: SdrProxy object
    """
    global _sdr_proxies
    return utils._register_proxy(_sdr_proxies, node_nbr, mem.SdrProxy, node_nbr)

def get_psm_proxy(node_nbr):
    """ Return a PsmProxy for a given client application. If it already exists, it
        gives you th ealready instantiated copy

        :param node_nbr: Node number
        :return: PsmProxy object
    """
    global _psm_proxies
    return utils._register_proxy(_psm_proxies, node_nbr, mem.PsmProxy, node_nbr)

def shutdown():
    """ Shutdowns pyion: All endpoints, access points, etc. """
    # Iterate through all BP endpoints and close them
    print('Closing all BP endpoints... ', end='')
    for proxy in _bp_proxies.values():
        proxy.bp_close_all()
    print('DONE')

    # Iterate through all CFDP entities and close them
    print('Closing all CFDP entities... ', end='')
    for proxy in _cfdp_proxies.values():
        proxy.cfdp_cancel_all()
        proxy.cfdp_close_all()
    print('DONE')

    # Iterate through all LTP access points and close them
    print('Closing all LTP access points...', end='')
    for proxy in _ltp_proxies.values():
        proxy.ltp_interrupt_all()
        proxy.ltp_close_all()
    print('DONE')

    # Iterate through all SDR proxies and close them
    print('Closing all SDR proxies...', end='')
    for proxy in _sdr_proxies.values():
        proxy.close()
    print('DONE')

    # Iterate through all PSM proxies and close them
    print('Closing all PSM proxies...', end='')
    for proxy in _psm_proxies.values():
        proxy.close()
    print('DONE')

# ============================================================================
# === Capture the SIGNIT and trigger cleanup
# ============================================================================

def pyion_sigint_handler(sig, frame):
    """ Default SIGINT handler. It closes all endpoints for all proxies and
        then detaches them from ION.
    """
    print('[pyion] KeyboardInterruption... ')
    shutdown()
    
# Register the signal handler function. If another one was provided previously,
# do not override it, combine it with pyion's
prev_sigint_handler = signal.getsignal(signal.SIGINT)
if prev_sigint_handler is None:
    signal.signal(signal.SIGINT, pyion_sigint_handler)
else:
    def combined_sigint_handler(sig, frame):
        pyion_sigint_handler(sig, frame)
        prev_sigint_handler(sig, frame)
    signal.signal(signal.SIGINT, combined_sigint_handler)
    
# ============================================================================
# === Proxy to BP in ION for a given node
# ============================================================================

class BpProxy(utils.Proxy):
    """ Proxy to the ION enginer running in this host. Do not instantiate 
        manually, use ``pyion.get_bp_proxy`` instead.

        Private Variables
        -----------------
        :ivar _ept_map: {'ipn:1.1': Endpoint_1, 'ipn:1.2': Endpoint_2, ...}
    """
    def __init__(self, node_nbr):
        """ Initializes the proxy, checks that the ION environment variables are
            valid, and attaches to the ION engine.

            :param int node_nbr: Node number for this proxy
        """
        global _bp_proxies

        # If a proxy for this node already exists, raise error
        if node_nbr in _bp_proxies:
            raise KeyError('A proxy for node {} already exists. Use ``pyion.get_bp_proxy`` \
                            to retrieve it'.format(node_nbr))

        # Call parent constructor
        super().__init__(node_nbr)      

        # Map {eid: Endpoint}
        self._ept_map = {}

    def __del__(self):
        """ Close all Endpoints associated with this proxy """
        global _bp_proxies
        self.bp_close_all()
        self.bp_detach()
        utils._unregister_proxy(_bp_proxies, self.node_nbr)

    def is_endpoint_open(self, eid):
        """ Check if this Proxy thinks a given EID is currently opened
            
            :param EID: EID
            :return: True if it is open
        """
        return eid in self._ept_map

    @property
    def open_endpoints(self):
        """ List all EIDs that are this Proxy sees open

            :return: Tuple of EIDS
        """
        return tuple(getattr(self, '_ept_map', {}).values())

    @utils.in_ion_folder
    def bp_attach(self):
        """ Attach to ION """
        # Attach to ION instance
        _bp.bp_attach()

        # Mark as attached to ION
        self.attached = True

    @utils.in_ion_folder
    def bp_detach(self):
        """ Dettach from ION """
        # Detach from ION instance
        _bp.bp_detach()

        # Mark as detached from ION
        self.attached = False

    @utils._chk_attached
    @utils.in_ion_folder
    def bp_open(self, eid, TTL=3600, priority=cst.BpPriorityEnum.BP_STD_PRIORITY,
                report_eid=None, custody=cst.BpCustodyEnum.NO_CUSTODY_REQUESTED,
                report_flags=cst.BpReportsEnum.BP_NO_RPTS, ack_req=cst.BpAckReqEnum.BP_NO_ACK_REQ,
                retx_timer=0, chunk_size=None, timeout=None, mem_ctrl=False):
        """ Open an endpoint. If it already exists, the existing instance
            is returned.

            .. Tip:: The parameters specified when opening the endpoint are
                     used as default values. When calling ``bp_send``, they
                     can be overriden.
            .. Tip:: To see longer explanation for these parameters, see
                     ``man bp`` in shell terminal

            :param EID: ID of the enpoint being opened
            :param TTL: Time-to-live [sec] for all bundles through this endpoint. 
                        Default is 3600 seconds
            :param priority: Class of service for all bundles through this endpoint.
                             Default is ``BpPriorityEnum.BP_STD_PRIORITY``
            :param report_eid: Endpoint id where report bundles will be sent to
            :param custody: Type of custody required for the bundles issued by this
                            endpoint. Default is ``BpCustodyEnum.NO_CUSTODY_REQUIRED``
            :param report_flags: Flag indicating which reports to request for a bundle.
                                 Default is ``BpReportsEnum.BP_NO_RPTS``. Example
                                 ``BpReportsEnum.BP_RECEIVED_RPT | BpReportsEnum.BP_CUSTODY_RPT``.
            :param ack_req: If True, it indicates that the application on top of BP is 
                            expecting to receive and end-to-end acknowledgement. Default
                            is ``BpAckReqEnum.BP_NO_ACK_REQ``
            :param retx_timer: Custodial timer retransmission in [sec]. Defaults to 0, which 
                               means that no timer is created.
            :param chunk_size: Send/Receive data in bundles of ``chunk_size`` bytes (plus header), 
                               instead of a single potentially very large bundle.
            :param timeout: If specified, the endpoint will be interrupted if timeout
                            seconds elapse without receiving anything.
            :return: Endpoint object
        """
        # If this EID is already open, return it
        if self.is_endpoint_open(eid):
            return self._ept_map[eid]

        # Detect if this endpoint "detains" bundles (see bp_open_source in ION manual)
        detained = (retx_timer > 0)

        # Open EID in ION. Get the address of the BpSapState as a long. If retx_timer>0, then
        # open the endpoint in "detained mode" (see bp_open_source in ION manual).
        sap_addr = _bp.bp_open(eid, int(detained), int(mem_ctrl))

        # Create an endpoint
        ept_obj = bp.Endpoint(self, eid, sap_addr, TTL, int(priority), report_eid,
                              int(custody), int(report_flags), int(ack_req), 
                              int(retx_timer), detained, chunk_size, timeout, mem_ctrl)

        # Store it
        self._ept_map[eid] = ept_obj

        # Return it
        return ept_obj

    @utils._chk_attached
    @utils.in_ion_folder
    def bp_close(self, ept):
        """ Close an endpoint. If it is not open, ``ConnectionError`` is raised.

            :param: Endpoint object to close
        """
        # If object passed is not endpoint, fail
        if not isinstance(ept, bp.Endpoint):
            raise ValueError('Expected an Enpoint instance, '+ str(type(ept)))

        # Interrupt this endpoint first. 
        try:
            self.bp_interrupt(ept)
        except ConnectionError:
            raise ConnectionError('Cannot close endpoint {}. It is not open.'.format(ept.eid))

        # Get SAP address in memory.
        ept_obj = self._ept_map.pop(ept.eid)

        # If already closed, you are done
        if not ept_obj.is_open:
            return

        # Close EID in ION
        _bp.bp_close(ept_obj._sap_addr)

        # Mark object as inactive
        ept_obj._cleanup()

    @utils._chk_attached
    @utils.in_ion_folder
    def bp_interrupt(self, ept):
        """ Interrupt and endpoint while receiving data. If it is not open, ``ConnectionError`` is raised.
            If this endpoint is not blocked receiving, this call has no effect.

            :param: Endpoint object to close
        """
        # If object passed is not endpoint, fail
        if not isinstance(ept, bp.Endpoint):
            raise ValueError('Expected an Enpoint instance')
        
        # If this EID is not open, return
        if not self.is_endpoint_open(ept.eid):
            raise ConnectionError('Cannot interrupt endpoint {}. It is not open.'.format(ept.eid))

        # Interrupt endpoint in ION
        _bp.bp_interrupt(ept._sap_addr)

        # This is little hack to ensure that SIGINT prints "interrupted" as opposed
        # to raising ConnectionAbortedError. However, this is not guaranteed
        sleep(0.001)

    def bp_close_all(self):
        """ Close all opened endpoints """
        # Close all endpoints
        for ept in self.open_endpoints:
            self.bp_close(ept)

    def bp_interrupt_all(self):
        """ Interrupt all opened endpoints """
        # Close all endpoints
        for ept in self.open_endpoints:
            self.bp_interrupt(ept)

# ============================================================================
# === Proxy to CFDP engine in ION for a given node
# ============================================================================

class CfdpProxy(utils.Proxy):
    def __init__(self, node_nbr):
        # Call parent constructor
        super().__init__(node_nbr)

        # Map {entity_nbr: Entity}
        self._ett_map = {}

    def __del__(self):
        """ Close all Endpoints associated with this proxy """
        global _cfdp_proxies
        self.cfdp_close_all()
        self.cfdp_detach()  
        utils._unregister_proxy(_cfdp_proxies, self.node_nbr)    

    def is_entity_open(self, peer_entity_nbr):
        """ Check if this CfdpProxy thinks a given Entity is currently opened
            
            :param EID: EID
            :return: True if it is open
        """
        return peer_entity_nbr in self._ett_map

    @property
    def open_entities(self):
        """ List all EIDs that are this Proxy sees open

            :return: Tuple of EIDS
        """
        return tuple(getattr(self, '_ett_map', {}).keys())

    @utils.in_ion_folder
    def cfdp_attach(self):
        """ Attach to ION's CFDP engine """
        # Attach to ION instance
        _cfdp.cfdp_attach()

        # Mark as attached to ION
        self.attached = True

    @utils.in_ion_folder
    def cfdp_detach(self):
        """ Dettach from ION """
        # Detach from ION instance
        _cfdp.cfdp_detach()

        # Mark as detached from ION
        self.attached = False

    @utils._chk_attached
    @utils.in_ion_folder
    def cfdp_open(self, peer_entity_nbr, endpoint, mode=cst.CfdpMode.CFDP_BP_RELIABLE,
                  closure_latency=cst.CfdpClosure.CFDP_NO_CLOSURE, 
                  seg_metadata=cst.CfdpMetadataEnum.CFDP_NO_SEG_METADATA):
        """ Open a CFDP entity

            :param peer_entity_nbr:
            :param endpoint: Enpoint object through which the CFDP will run
            :param mode: Reliable vs. Non-reliable. See all options in ``pyion.constants``
            :param closure_latency: See ``help(CfdpClosure)``
            :param seg_metadata:
        """ 
        # If already open, return it
        if peer_entity_nbr in self._ett_map:
            return self._ett_map[peer_entity_nbr]

        # Open a proxy to the CFDP engine
        param_addr = _cfdp.cfdp_open(
            peer_entity_nbr, 
            endpoint.TTL,
            endpoint.priority,
            endpoint.sub_priority,
            endpoint.report_flags,
            endpoint.criticality,
        )

        # Create a CFDP entity object
        ett_obj = cfdp.Entity(self, peer_entity_nbr, param_addr, endpoint,
                              int(mode), int(closure_latency), int(seg_metadata))

        # Store it
        self._ett_map[peer_entity_nbr] = ett_obj

        # Return it
        return ett_obj

    @utils._chk_attached
    @utils.in_ion_folder
    def cfdp_close(self, peer_entity_nbr):
        """ Close a CFDP entity

            :param peer_entity_nbr:
        """ 
        # Get SAP address in memory.
        ett_obj = self._ett_map.pop(peer_entity_nbr)

        # If already closed, you are done
        if not ett_obj.is_open:
            return

        # Close Entity in ION
        _cfdp.cfdp_close(ett_obj._param_addr)

        # Mark object as inactive
        ett_obj._cleanup()

    def cfdp_close_all(self):
        """ Close all open entities in this node """
        for peer_entity_nbr in self.open_entities:
            self.cfdp_close(peer_entity_nbr)

    def cfdp_cancel_all(self):
        """ Cancel any transaction in all entities in this node """
        for peer_nbr in self.open_entities:
            self._ett_map[peer_nbr].cfdp_cancel()

# ============================================================================
# === Proxy to LTP engine in ION for a given node
# ============================================================================

class LtpProxy(utils.Proxy):
    def __init__(self, node_nbr):
        # Call parent constructor
        super().__init__(node_nbr)

        # Map {cliend id: AccessPoint}
        self._sap_map = {}

    def __del__(self):
        """ Close all access points associated with this proxy """
        global _ltp_proxies
        self.ltp_close_all()
        self.ltp_detach()
        utils._unregister_proxy(_ltp_proxies, self.node_nbr)

    def is_client_open(self, client_id):
        """ Check if this LtpProxy thinks a given client is already using the
            LTP engine
            
            :param client_id: The id of the client application (int)
            :return: True if it is open
        """
        return client_id in self._sap_map

    @property
    def open_clients(self):
        """ List all client ids that are this Proxy sees open

            :return: Tuple of client ids
        """
        return tuple(getattr(self, '_sap_map', {}).keys())

    @utils.in_ion_folder
    def ltp_attach(self):
        """ Attach to ION's LTP engine """
        # Attach to ION instance
        _ltp.ltp_attach()

        # Mark as attached to ION
        self.attached = True

    @utils.in_ion_folder
    def ltp_detach(self):
        """ Dettach from ION """
        # Detach from ION's LTP engine
        _ltp.ltp_detach()

        # Mark as detached from ION
        self.attached = False

    @utils._chk_attached
    @utils.in_ion_folder
    def ltp_open(self, client_id):
        """ Open a service access point for a client 

            :param client_id:
        """ 
        # If already open, return it
        if client_id in self._sap_map:
            return self._sap_map[client_id]

        # Open ltp access point
        sap_addr = _ltp.ltp_open(client_id)

        # Create an LTP Access Point
        sap_obj = ltp.AccessPoint(self, client_id, sap_addr)

        # Store it
        self._sap_map[client_id] = sap_obj

        # Return it
        return sap_obj

    @utils._chk_attached
    @utils.in_ion_folder
    def ltp_close(self, client_id):
        """ Close an LTP access point

            :param client_id:
        """
        # Get SAP address in memory.
        sap_obj = self._sap_map.pop(client_id)

        # If already closed, you are done
        if not sap_obj.is_open:
            return

        # Close Entity in ION
        _ltp.ltp_close(sap_obj._sap_addr)

        # Mark object as inactive
        sap_obj._cleanup()

    def ltp_close_all(self):
        """ Close all open clients in this proxy """
        for client_id in self.open_clients:
            self.ltp_close(client_id)

    def ltp_interrupt_all(self):
        """ Interrupt any LTP transactions in all clients in this proxy """
        for client_id in self.open_clients:
            self._sap_map[client_id].ltp_interrupt()

# ============================================================================
# === EOF
# ============================================================================
