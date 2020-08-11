#!/usr/bin/python3

"""
# Sends a stream of bundles to the receiver until user terminates
# with Ctrl+C
#
# Author: Marc Sanchez Net
# Date:   04/15/2019
# Copyright 2019 (c), Jet Propulsion Laboratory
"""

# Fix path (ONLY FOR THIS TEST)
import sys
sys.path.append('..')
sys.path.append('../pyion')

from datetime import datetime
from pprint import pprint
from threading import Thread
import time

# Import module
import pyion

# =================================================================
# === Define global variables
# =================================================================

# ION node number
node_nbr = 1

# Originating and destination endpoints
orig_eid = 'ipn:1.1'
dest_eid = 'ipn:2.1'

# [Hz] Rate at which the SDR is monitored
rate = 100

# [sec] Timespan of consecutive measurements
timespan = 5

# =================================================================
# === MAIN
# =================================================================

# Create a proxy to ION, the SDR and the PSM
proxy = pyion.get_bp_proxy(node_nbr)
sdr   = pyion.get_sdr_proxy(node_nbr)
psm   = pyion.get_psm_proxy(node_nbr)

# Start monitoring
sdr.start_monitoring(rate=rate, timespan=timespan, print_results=True)
psm.start_monitoring(rate=rate, timespan=timespan, print_results=True)

# Open a endpoint and set its properties. Then send file
with proxy.bp_open(orig_eid) as eid:
    while True:
        try:
            msg = '{} - {}'.format(datetime.now(), 'a'*100)
            eid.bp_send(dest_eid, msg)
            time.sleep(0.1)
        except MemoryError:
            summary, small_pool, large_pool = sdr.dump()
            print(summary)
            raise
        except InterruptedError:
            break

# Get SDR results
sdr_res = sdr.stop_monitoring()
pprint(sdr_res)

# Get PSM results
psm_res = psm.stop_monitoring()
pprint(psm_res)

# =================================================================
# === EOF
# =================================================================
