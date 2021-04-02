#!/usr/bin/python3

"""
# Opens endpoint 'ipn:2.1' and receives data until user issues SIGINT
# Data received is simply printed to stdout.
#
# Author: Marc Sanchez Net
# Date:   04/15/2019
# Copyright 2019 (c), Jet Propulsion Laboratory
"""

# Fix path for this test
import sys
sys.path.append('..')
sys.path.append('../pyion')

# General imports
from datetime import datetime
from pprint import pprint

# Import module
import pyion
from pyion import CfdpEventEnum

# =================================================================
# === Define global variables
# =================================================================

# ION node number
node_nbr = 2
peer_nbr = 1

# Endpoint to listen to
EID = 'ipn:2.1'

# Timing-related variables
tfmt = '%m-%d-%Y %H:%M:%S.%f'
t_tx = datetime.utcnow()

# Channel-related properties
ch_err_prob = None

# =================================================================
# === Define event handlers
# =================================================================

def cfdp_event_handler(ev_type, ev_params):
    print('-'*80)
    print('Event type:', ev_type)
    print('Event parameters')
    pprint(ev_params)
    print('-'*80)
    print('')

def cfdp_metadata_handler(ev_type, ev_params):    
    # Record the time at which the file was sent
    global t_tx, ch_err_prob
    try:
        usr_msgs = ev_params['user_messages']
        if len(usr_msgs) > 0:
            t_tx        = datetime.strptime(usr_msgs[0], tfmt)
            ch_err_prob = float(usr_msgs[1])
    except:
        pass

# =================================================================
# === MAIN
# =================================================================

# Create a proxy to ION
bpxy = pyion.get_bp_proxy(node_nbr)
cpxy = pyion.get_cfdp_proxy(node_nbr)

# Open a endpoint and set its properties.
ept = bpxy.bp_open(EID)

# Open a CFDP entity to Node 2 and send file and register a handler
ett = cpxy.cfdp_open(peer_nbr, ept)
ett.register_event_handler(CfdpEventEnum.CFDP_ALL_IND, cfdp_event_handler)
ett.register_event_handler(CfdpEventEnum.CFDP_METADATA_RECV_IND, cfdp_metadata_handler)

# Wait until the next transaction ends
print('Waiting to receive new file...')
if ett.wait_for_transaction_end():
    t_arr = datetime.utcnow()
    print('CHANNEL ERROR PROBABILITY:', ch_err_prob)
    print('FILE TRANSMISSION TIME:', t_tx)
    print('FILE RECEPTION TIME:', t_arr)
    print('TOTAL ELAPSED TIME:', t_arr-t_tx)