#!/usr/bin/python3

"""
# Sends a stream of bundles via BP
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
rept_eid = 'ipn:1.2'

# Define endpoint properties
ept_props = {
    'TTL':          3600,   # [sec]
    #'custody':      BpCustodyEnum.SOURCE_CUSTODY_REQUIRED,
    'priority':     BpPriorityEnum.BP_EXPEDITED_PRIORITY,
    #'report_eid':   rept_eid,
    #'report_flags': BpReportsEnum.BP_RECEIVED_RPT
    #'report_flags': BpReportsEnum.BP_RECEIVED_RPT | BpReportsEnum.BP_CUSTODY_RPT,
    'mem_ctrl':     True
}

# Create a proxy to ION
proxy = pyion.get_bp_proxy(node_nbr)

# =================================================================
# === Acquire reports
# =================================================================

# Open endpoint to get reports
rpt_eid = proxy.bp_open(rept_eid)

def print_reports():
    while True:
        try:
            data = rpt_eid.bp_receive()
            print(data)
        except InterruptedError:
            break
        
# Start monitoring thread
th = Thread(target=print_reports, daemon=True)
th.start()

# =================================================================
# === MAIN
# =================================================================

def send_data(ept):
    # Counter of total bytes sent
    total_sent = 0

    for i in range(500):
        msg = str(datetime.now()) + ' - ' + 'a'*10000
        print('{}) Sending {} bytes'.format(i+1, sys.getsizeof(msg)))
        ept.bp_send(dest_eid, msg)
        total_sent += sys.getsizeof(msg)

    print('Total sent = {} bytes'.format(total_sent))

# Open a endpoint and set its properties. Then send file
with proxy.bp_open(orig_eid, **ept_props) as ept:
    send_data(ept)
    
# Sleep for a while and stop the monitoring thread
time.sleep(2)
proxy.bp_interrupt(rept_eid)
th.join()

# =================================================================
# === EOF
# =================================================================
