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

# Create a proxy to ION
proxy = pyion.get_bp_proxy(node_nbr)

# Attach to ION
proxy.bp_attach()

# Create proxy to SDR and PSM
sdr = pyion.get_sdr_proxy(1)
psm = pyion.get_psm_proxy(1)

# =================================================================
# === MAIN
# =================================================================

# Start monitoring
sdr.start_monitoring(rate=1, timespan=10, print_results=True)
psm.start_monitoring(rate=1, timespan=10, print_results=True)

# Open a endpoint and set its properties. Then send file
with proxy.bp_open(orig_eid, mem_ctrl=True) as eid:
    try:
        for i in range(5000):
            eid.bp_send(dest_eid, str(datetime.now()) + ' - ' + 'a'*1000)
    except MemoryError:
        summary, small_pool, large_pool = sdr.dump()
        print(summary)
        raise

# Get SDR results
sdr_res = sdr.stop_monitoring()
#sdr_sum = pd.DataFrame.from_dict(sdr_res['summary']).T
#sdr_spl = pd.DataFrame.from_dict(sdr_res['small_pool'], orient='index')
#sdr_lpl = pd.DataFrame.from_dict(sdr_res['large_pool'], orient='index')

# Get PSM results
psm_res = psm.stop_monitoring()
#psm_sum = pd.DataFrame.from_dict(sdr_res['summary']).T
#psm_spl = pd.DataFrame.from_dict(sdr_res['small_pool'], orient='index')
#psm_lpl = pd.DataFrame.from_dict(sdr_res['large_pool'], orient='index')

# =================================================================
# === EOF
# =================================================================
