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
from threading import Thread
import time

# Import module
import pyion
from pyion import BpCustodyEnum, BpPriorityEnum, BpReportsEnum

# =================================================================
# === Define global variables
# =================================================================

# Define file to send
file = 'bping'
#file = 'test_file.pptx'

# ION node number
node_nbr = 1

# Originating and destination endpoints
orig_eid = 'ipn:1.1'
dest_eid = 'ipn:2.1'
rept_eid = 'ipn:1.2'

# Define endpoint properties
ept_props = {
    'TTL':          3600,   # [sec]
    'custody':      BpCustodyEnum.SOURCE_CUSTODY_REQUIRED,
    'priority':     BpPriorityEnum.BP_EXPEDITED_PRIORITY,
    'report_eid':   rept_eid,
    'report_flags': BpReportsEnum.BP_RECEIVED_RPT
    #'report_flags': BpReportsEnum.BP_RECEIVED_RPT | BpReportsEnum.BP_CUSTODY_RPT,
    #'report_flags': BpReportsEnum.BP_NO_RPTS,
    #'chunk_size':   10   # [bytes]
}

# Create a proxy to ION
proxy = pyion.get_bp_proxy(node_nbr)

# Attach to ION
proxy.bp_attach()

# =================================================================
# === Send data
# =================================================================

# Open endpoint to get reports
eid = proxy.bp_open(orig_eid)

def send_data():
    while True:
        try:
            eid.bp_send(dest_eid, str(datetime.now()) + ' - ' + 'a'*100)
            time.sleep(0.5)
        except KeyboardInterrupt:
            break
        
# Start monitoring thread
th = Thread(target=send_data, daemon=True)
th.start()

# =================================================================
# === Print state of contact plan
# =================================================================

def monitor_cp():
    while True:
        try:
            pprint(pyion.cgr_list_contacts())
            time.sleep(1)
            print('')
        except KeyboardInterrupt:
            break
            
# Start monitoring thread
th2 = Thread(target=monitor_cp, daemon=True)
th2.start()

# =================================================================
# === MAIN
# =================================================================

# Wait for a while
time.sleep(2.5)

# Update the contact plan
ts, te = 0, 10
for i in range(10):
    pyion.cgr_add_contact(0, 1, 3, '+{}'.format(ts), '+{}'.format(te), 10000)
    time.sleep(1)
    ts, te = ts+20, te+20

# Wait for a while
time.sleep(10)

# =================================================================
# === EOF
# =================================================================
