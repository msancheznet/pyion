""" 
# ===========================================================================
# Defines proxies to ION's SDR and PSM so that you can monitor them.
#
# Author: Marc Sanchez Net
# Date:   08/12/2019
# Copyright (c) 2019, California Institute of Technology ("Caltech").  
# U.S. Government sponsorship acknowledged.
# ===========================================================================
"""

# Generic imports
import abc
from collections import defaultdict
from mock import Mock
from threading import Thread
import time
from warnings import warn

# Import pyion modules
import pyion.utils as utils

# Import C Extension
try:
	import _mem
except ImportError:
	warn('_mem extension not available. Using mock instead.')
	_mem = Mock()

# Define all methods/vars exposed at pyion
__all__ = ['SdrProxy', 'PsmProxy']

# ============================================================================
# === Abstract Memory Proxy
# ============================================================================

class MemoryProxy(utils.Proxy, abc.ABC):
    """ Abstract class parent to SdrProxy and PsmProxy """
    def __init__(self, node_nbr):
        # Call parent constructor
        super().__init__(node_nbr)

        # Variables for monitor
        self._th         = None
        self._monitor_on = False
        self._rate       = None
        self._results    = None

    @abc.abstractmethod
    def dump(self):
        """ Dump memory state """
        pass

    def close(self):
        # If no monitoring thread, return
        if self._th is None: return

        # If monitoring thread is not active, return
        if not self._th.is_alive(): return
        
        # Wait for thread to stop
        self._monitor_on = False
        self._th.join()

        # Delete results
        del self._results

    def start_monitoring(self, rate=1):
        """ Start monitoring the SDR for a while
            
            :param int rate: Rate in [Hz]
        """
        # If a monitoring session is already on, error
        if self._monitor_on:
            raise ValueError('Memory is already being monitored.')

        # Prepare monitoring session
        self._rate       = utils.Rate(rate)
        self._monitor_on = True
        self._results    = defaultdict(dict)

        # Start monitor in separate thread
        self._th = Thread(target=self._start_monitoring, daemon=True)
        self._th.start()

    def _start_monitoring(self):
        while self._monitor_on:
            # Measurement index
            t = time.time()

            # Take measurement
            try:
                summary, small_pool, large_pool = self.dump()
                self._results['summary'][t]    = summary
                self._results['small_pool'][t] = small_pool
                self._results['large_pool'][t] = large_pool
            except Exception as e:
                self._results['summary'][t]    = e
                self._results['small_pool'][t] = None
                self._results['large_pool'][t] = None

            # Sleep for a while
            self._rate.sleep()

    def stop_monitoring(self):
        """ Stop monitoring the SDR and return results"""
        # If no monitoring session is on, return
        if not self._monitor_on: return

        # Signal exit to the monitor thread
        self._monitor_on = False

        # Wait for monitoring thread to exit
        self._th.join()

        # Return collected results
        return self._results


# ============================================================================
# === SDR PROXY
# ============================================================================

class SdrProxy(MemoryProxy):
    """ Proxy object to ION's SDR 

        :ivar int: Node number
        :ivar str: SDR name (see ``ionconfig``, option ``sdrName``)
    """
    def __init__(self, node_nbr, sdr_name):
        # Call parent constructor
        super().__init__(node_nbr)

        # Store SDR name
        self.sdr_name = sdr_name

    @utils.in_ion_folder
    def dump(self):
        """ Dump the current state of the SDR 
        
        :return Tuple[Dict]: usage summary, small pool free block count,
                             large pool free block count
        """
        return _mem.sdr_dump(self.sdr_name)

    def __str__(self):
        return '<{}: {}>'.format(self.__class__.__name__, self.sdr_name)

# ============================================================================
# === PSM PROXY
# ============================================================================

class PsmProxy(MemoryProxy):
    """ Proxy object to ION's PSM 
    
        :ivar int wm_key: See ``wmKey`` in ``ionconfig``
        :ivar int wm_size: Unused
        :ivar str partition_name: Unused
    """
    def __init__(self, node_nbr, wm_key, wm_size=0, partition_name=-1):
        # Call parent constructor
        super().__init__(node_nbr)

        # Store variables to identify the PSM
        self.wm_key   = int(wm_key)
        self._wm_size = int(wm_size)
        self._part_id = str(wm_key)

    @utils.in_ion_folder
    def dump(self):
        """ Dump the current state of the PSM
        
            :return Tuple[Dict]: usage summary, small pool free block count,
                                 large pool free block count
        """
        return _mem.psm_dump(self.wm_key, self._wm_size, self._part_id)

    def __str__(self):
        return '<{}: {}>'.format(self.__class__.__name__, self.wm_key)