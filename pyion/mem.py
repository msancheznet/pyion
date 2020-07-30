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
from pprint import pprint
from unittest.mock import Mock
from threading import Thread
import time
from warnings import warn

# Import pyion modules
import pyion.utils as utils

# Import C Extension
import _mem

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
        self._tspan      = -1
        self._print_res  = False
        self._results    = defaultdict(dict)

    @abc.abstractmethod
    def dump(self):
        """ Dump memory state """
        pass

    def clean_results(self):
        """ Clean all stored results """
        self._results = defaultdict(dict)

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

    def start_monitoring(self, rate=1, timespan=-1, print_results=False):
        """ Start monitoring the SDR for a while
            
            :param int rate: Rate in [Hz]
            :param float timespan: Span of time during which measurements
                                   are taken. After that, the measurements
                                   are deleted and new data is acquired.
                                   The default value of -1 indicates that
                                   measurements are never deleted.
            :param float print_results: If True, the state of the memory is printed
                                        to stdout prior to being deleted
        """
        # If a monitoring session is already on, error
        if self._monitor_on:
            raise ValueError('Memory is already being monitored.')

        # Prepare monitoring session
        self._rate       = utils.Rate(rate)
        self._monitor_on = True
        self._results    = defaultdict(dict)
        self._tspan      = timespan
        self._print_res  = print_results

        # Start monitor in separate thread
        self._th = Thread(target=self._start_monitoring, daemon=True)
        self._th.start()

    def _start_monitoring(self):
        # Initialize variables
        tic = time.time()
        summary = {}

        while self._monitor_on:
            # Measurement index
            t = time.time()

            # If enough time has elapsed, clear the results
            if self._tspan > 0 and time.time()-tic >= self._tspan:
                # Print if necessary
                if self._print_res:
                    print('Num. measurements:', len(self._results['summary']))
                    pprint(summary)

                # Clean results and restart time counter
                self.clean_results()
                tic = time.time()

            # Take measurement
            try:
                summary, small_pool, large_pool = self.dump()
            except Exception as e:
                summary, small_pool, large_pool = e, None, None
            
            # Store results
            self._results['summary'][t]    = summary
            self._results['small_pool'][t] = small_pool
            self._results['large_pool'][t] = large_pool

            # Sleep for a while
            if self._rate: self._rate.sleep()

    def stop_monitoring(self):
        """ Stop monitoring the SDR and return results"""
        # If no monitoring session is on, return
        if not self._monitor_on: return

        # Signal exit to the monitor thread
        self._monitor_on = False
        self._rate = None
        self._tspan = -1

        # Wait for monitoring thread to exit
        self._th.join()

        # Return collected results
        return self._results

    def __str__(self):
        return '<{}>'.format(self.__class__.__name__)

# ============================================================================
# === SDR PROXY
# ============================================================================

class SdrProxy(MemoryProxy):
    """ Proxy object to ION's SDR 

        :ivar int: Node number
        :ivar str: SDR name (see ``ionconfig``, option ``sdrName``)
    """
    @utils.in_ion_folder
    def dump(self):
        """ Dump the current state of the SDR 
        
        :return Tuple[Dict]: usage summary, small pool free block count,
                             large pool free block count
        """
        return _mem.sdr_dump()

# ============================================================================
# === PSM PROXY
# ============================================================================

class PsmProxy(MemoryProxy):
    """ Proxy object to ION's PSM 
    
        :ivar int wm_key: See ``wmKey`` in ``ionconfig``. Defaults to 65281 
                          (i.e., ION's default value)
        :ivar int wm_size: Unused
        :ivar str partition_name: Unused
    """
    @utils.in_ion_folder
    def dump(self):
        """ Dump the current state of the PSM
        
            :return Tuple[Dict]: usage summary, small pool free block count,
                                 large pool free block count
        """
        return _mem.psm_dump()