#!/usr/bin/python3

"""
# Opens endpoint 'ipn:2.1' and receives data until user issues SIGINT
# Data received is simply printed to stdout.
#
# Author: Marc Sanchez Net
# Date:   04/15/2019
# Copyright 2019 (c), Jet Propulsion Laboratory
"""

# General imports
import ast
from datetime import datetime
import json
import hashlib
import os
from pprint import pprint

# Pyion imports
import pyion
from pyion import BpCustodyEnum, BpPriorityEnum, BpReportsEnum
from pyion import CfdpMode, CfdpEventEnum

# ===================================================================================
# === Define global variables
# ===================================================================================

# Set environment variables for ION
pyion.ION_NODE_LIST_DIR = './nodes'

# ION node number
node_nbr = 2
peer_nbr = 1

# Endpoint for CFDP connections, and endpoint for reporting
dst_eid = 'ipn:2.1'
rpt_eid = 'ipn:2.2'

# Timing-related variables
tfmt = '%m-%d-%Y %H:%M:%S.%f'

# ===================================================================================
# === Create proxies for tests
# ===================================================================================

# Create a proxies to ION
bpxy = pyion.get_bp_proxy(node_nbr)
cpxy = pyion.get_cfdp_proxy(node_nbr)

# Attach to ION's BP and CFDP
bpxy.bp_attach()
cpxy.cfdp_attach()

# ===================================================================================
# === Open CFDP engine and BP endpoints
# ===================================================================================

# Open a CFDP entity to Node 1
ett = cpxy.cfdp_open(peer_nbr, bpxy.bp_open(dst_eid))

# Open endpoint for reporting
ept = bpxy.bp_open(rpt_eid)

# ===================================================================================
# === Define handlers for CFDP events
# ===================================================================================

# Initialize variables
cur_mtd = {}
cur_mtd['is_last'] = False

def cfdp_event_handler(ev_type, ev_params):
    print('-'*80)
    print('Event type:', ev_type)
    print('Event parameters')
    pprint(ev_params)
    print('-'*80)
    print('')

def cfdp_metadata_handler(ev_type, ev_params):
    global cur_mtd    
    cur_mtd = json.loads(ev_params['user_messages'][0])

def cfdp_tx_finished_handler(ev_type, ev_params):
    # Measure time transaction finished
    t_arr = datetime.utcnow()

    # Read file
    with open(cur_mtd['file'], 'rb') as f:
        message = f.read()

    # Delete file
    os.remove(cur_mtd['file'])

    # Compute hash of the received file
    m = hashlib.sha256()
    m.update(bytes(message))
    r_hash = m.hexdigest()

    # Create result
    data = {'rx_end': t_arr.strftime(tfmt),
            'per': cur_mtd['per'],
            'ok': cur_mtd['hash'] == r_hash,
            'is_last': cur_mtd['is_last']}

    # Send result
    ept.bp_send('ipn:1.2', json.dumps(data))
    print('Transaction finished')
    print(data)

# Register event handlers
ett.register_event_handler(CfdpEventEnum.CFDP_ALL_IND, cfdp_event_handler)
ett.register_event_handler(CfdpEventEnum.CFDP_METADATA_RECV_IND, cfdp_metadata_handler)
ett.register_event_handler(CfdpEventEnum.CFDP_TRANSACTION_FINISHED_IND, 
                           cfdp_tx_finished_handler)

# ===================================================================================
# === RUN EXPERIMENT
# ===================================================================================

# Wait for next CFDP transaction to arrive
while not cur_mtd['is_last']:
    ett.wait_for_transaction_end()
