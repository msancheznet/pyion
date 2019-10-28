""" 
# ===========================================================================
# Interface between the ION's implementation of the CCSDS LTP and Python. 
# Internally, all classes call the C Extension _ltp.
#
# For detailed documentation, see __init__.py
#
# Author: Marc Sanchez Net
# Date:   05/30/2019
# Copyright (c) 2019, Jet Propulstion Laboratory
# ===========================================================================
"""

# General imports
from pathlib import Path
from threading import Event, Thread

# Module imports
import pyion
import pyion.utils as utils

# Import C Extension
import _ltp

# Define all methods/vars exposed at pyion
__all__ = ['AccessPoint']

# ============================================================================
# === AccessPoint class
# ============================================================================

class AccessPoint():
    """ Class to represent an LTP access point. """
    def __init__(self, proxy, client_id, sap_addr):
        # Store variables
        self.proxy     = proxy
        self.client_id = client_id
        self._sap_addr = sap_addr
        self.node_dir  = proxy.node_dir
        self._result   = None

    def __del__(self):
        # If you have already been closed, return
        if not self.is_open:
            return

        # Close the Entity and free C memory
        self.proxy.ltp_close(self.client_id)

    @property
    def is_open(self):
        """ Returns True if the access point is opened """
        return (self.proxy is not None and self._sap_addr is not None)

    def _cleanup(self):
        """ Clean access point after closing. Do not call directly,
            use ``proxy.ltp_close``
        """
        # Clear variables
        self.proxy     = None
        self.client_id = None
        self._sap_addr = None

    @utils._chk_is_open
    @utils.in_ion_folder
    def ltp_send(self, dest_engine_nbr, data):
        """ Trigger LTP to send data 

            :param: Destination engine number
            :param: Data as str, bytes or bytearray
        """
        self._result = _ltp.ltp_send(self._sap_addr, dest_engine_nbr, data)

    @utils._chk_is_open
    def ltp_receive(self):
        """ Trigger LTP to receive data """
        # Receive on another thread. Otherwise you cannot handle a SIGINT
        th = Thread(target=self._ltp_receive, daemon=True)
        th.start()
        th.join()

        # If exception, raise it
        if isinstance(self._result, Exception):
            raise self._result

        return self._result
        
    @utils.in_ion_folder
    def _ltp_receive(self):
        # Get payload from next bundle. If exception, return it too
        try:
            self._result = _ltp.ltp_receive(self._sap_addr)
        except Exception as e:
            self._result = e
            
    @utils._chk_is_open
    @utils.in_ion_folder
    def ltp_interrupt(self):
        """ Trigger LTP to be interrupted """
        _ltp.ltp_interrupt(self._sap_addr)

    def __enter__(self):
        """ Allows an endpoint to be used as context manager """
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        """ Allows an endpoint to be used as context manager """
        pass

    def __str__(self):
        return '<AccessPoint: {} ({})>'.format(self.client_id, 'Open' if self.is_open else 'Closed')

    def __repr__(self):
        return '<AccessPoint: {} ({})>'.format(self.client_id, self._sap_addr)