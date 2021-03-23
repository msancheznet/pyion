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
from threading import Thread, current_thread
import time
import json

# Import module
import pyion
from pyion import BpCustodyEnum, BpPriorityEnum, BpReportsEnum

# =================================================================
# === Define global variables
# =================================================================

# ION node number
node_nbr = 5

# Originating and destination endpoints
orig_eid = 'ipn:1.1'
dest_eid = 'ipn:2.1'
rept_eid = 'ipn:1.2'

# Number of threads
num_threads = 20

# Define endpoint properties
ept_props = {
    'TTL':          10,   # [sec]
    #'custody':      BpCustodyEnum.SOURCE_CUSTODY_REQUIRED,
    'priority':     BpPriorityEnum.BP_EXPEDITED_PRIORITY,
    #'report_eid':   rept_eid,
    #'report_flags': BpReportsEnum.BP_RECEIVED_RPT
    #'report_flags': BpReportsEnum.BP_RECEIVED_RPT | BpReportsEnum.BP_CUSTODY_RPT,
    'mem_ctrl':     True
}

# Create a proxy to ION
proxy = pyion.get_bp_proxy(node_nbr)

# Data endpoint
data_ept = proxy.bp_open(orig_eid, **ept_props)

# Open endpoint to get reports
rpt_ept = proxy.bp_open(rept_eid)

# =================================================================
# === Acquire reports
# =================================================================

def print_reports():
    while True:
        try:
            data = rpt_ept.bp_receive()
            print(data)
        except InterruptedError:
            break

# =================================================================
# === Functions to send data
# =================================================================

# Counter of total bytes sent
total_sent = 0

def send_data(ept):
    global total_sent
    
    for i in range(10):
        msg = json.dumps({
            'time': datetime.now().strftime('%Y-%m-%d %H:%M:%S.%f'),
            'thread_id': current_thread().name,
            'payload': 'a'*100
        })
        print('[{}/{}] Sending {} bytes'.format(current_thread().name, i+1, sys.getsizeof(msg)))
        ept.bp_send(dest_eid, msg)
        time.sleep(0.01)
        total_sent += sys.getsizeof(msg)    # NOT thread safe, but we don't care

def send_data_thread(ept):
    th = Thread(target=send_data, args=(ept,), daemon=True)
    th.start()
    return th

# =================================================================
# === Start threads
# =================================================================

# Start monitoring thread
#rpt_th = Thread(target=print_reports, daemon=True)
#rpt_th.start()

# Start threads
threads = [send_data_thread(data_ept) for _ in range(num_threads)]

# =================================================================
# === Start threads
# =================================================================

# Join all data threads
for th in threads:
    th.join()
print('Total sent = {} bytes'.format(total_sent))    
    
# Stop the reporitng thread
#proxy.bp_interrupt(rept_eid)
#rpt_th.join()

# =================================================================
# === EOF
# =================================================================
