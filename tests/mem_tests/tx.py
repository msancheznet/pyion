#!/usr/bin/python3

"""
# Sends 'bping' from ipn:1.1 to ipn:2.1. Use ``text_rx``
# at the receiver
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
import pandas as pd
from threading import Thread
import time

# Import module
import pyion
from pyion import BpCustodyEnum, BpPriorityEnum, BpReportsEnum

# =================================================================
# === Define global variables
# =================================================================

# ION node number
node_nbr = 1

# Originating and destination endpoints
orig_eid = 'ipn:1.1'
dest_eid = 'ipn:2.1'

# [Hz] Rate at which the SDR is monitored
rate = 10

# =================================================================
# === MAIN
# =================================================================

# Create a proxy to ION, SDR and PSM
proxy = pyion.get_bp_proxy(node_nbr)
sdr   = pyion.get_sdr_proxy(node_nbr)
psm   = pyion.get_psm_proxy(node_nbr)

# Start monitoring
sdr.start_monitoring(rate=rate, timespan=1, print_results=True)
psm.start_monitoring(rate=rate, timespan=1, print_results=True)

# Open a endpoint and set its properties. Then send file
with proxy.bp_open(orig_eid, mem_ctrl=True) as eid:
    try:
        for i in range(1000):
            eid.bp_send(dest_eid, str(datetime.now()) + ' - ' + 'a'*100)
            time.sleep(0.01)
    except MemoryError:
        summary, small_pool, large_pool = sdr.dump()
        print(summary)
        raise

# Get SDR results
sdr_res = sdr.stop_monitoring()
#pprint(sdr_res)
sdr_sum = pd.DataFrame.from_dict(sdr_res['summary']).T
sdr_spl = pd.DataFrame.from_dict(sdr_res['small_pool'], orient='index')
sdr_lpl = pd.DataFrame.from_dict(sdr_res['large_pool'], orient='index')

# Get PSM results
psm_res = psm.stop_monitoring()
#pprint(psm_res)
psm_sum = pd.DataFrame.from_dict(sdr_res['summary']).T
psm_spl = pd.DataFrame.from_dict(sdr_res['small_pool'], orient='index')
psm_lpl = pd.DataFrame.from_dict(sdr_res['large_pool'], orient='index')

# =================================================================
# === EOF
# =================================================================
