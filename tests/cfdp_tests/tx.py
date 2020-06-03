#!/usr/bin/python3

"""
# Sends 'bping' from ipn:1.1 to ipn:2.1 using CFDP
#
# Author: Marc Sanchez Net
# Date:   04/15/2019
# Copyright 2019 (c), Jet Propulsion Laboratory
"""

# Fix path (ONLY FOR THIS TEST)
import sys
sys.path.append('..')
sys.path.append('../pyion')

# General imports
from datetime import datetime
from pprint import pprint
from time import sleep

# Import network emulation utility
import pynetem as netem

# Import module
import pyion
from pyion import BpCustodyEnum, BpPriorityEnum, BpReportsEnum
from pyion import CfdpEventEnum, CfdpFileStoreEnum

# =================================================================
# === Define global variables
# =================================================================

# Define file to send
file = './nodes/1/data/test_file.pptx'

# ION node numbers
node_nbr = 1
peer_nbr = 2

# Originating and destination endpoints
orig_eid = 'ipn:1.1'
dest_eid = 'ipn:2.1'

# Define endpoint properties
ept_props = {
    'TTL':          3600,   # [sec]
    'custody':      BpCustodyEnum.NO_CUSTODY_REQUESTED,
    'priority':     BpPriorityEnum.BP_STD_PRIORITY,
    'report_flags': BpReportsEnum.BP_NO_RPTS,
}

# Timing-related variables
tfmt = '%m-%d-%Y %H:%M:%S.%f'
t_tx = None
t_rx = None

# Channel-related properties
ch_delay    = 250
ch_jitter   = 50
ch_err_prob = 0.01

# =================================================================
# === Configure network
# =================================================================

# Create proxy to netem
#npxy = netem.Proxy('eth0')

# Add a delay and loss rule
#npxy.add_delay_rule(delay=ch_delay, jitter=ch_jitter, units='ms')
#npxy.add_bernoulli_loss_rule(error_prob=ch_err_prob)

# Commit rules
#npxy.commit_rules()
#print('Channel error probability = {}.'.format(ch_err_prob))

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

# =================================================================
# === MAIN
# =================================================================

# Create a proxies to ION
bpxy = pyion.get_bp_proxy(node_nbr)
cpxy = pyion.get_cfdp_proxy(node_nbr)

# Attach to ION's BP and CFDP
bpxy.bp_attach()
cpxy.cfdp_attach()

# Open a endpoint and set its properties.
ept = bpxy.bp_open(orig_eid, **ept_props)

# Open a CFDP entity to Node 2 and send file and register a handler
ett = cpxy.cfdp_open(peer_nbr, ept)
ett.register_event_handler(CfdpEventEnum.CFDP_ALL_IND, cfdp_event_handler)

# Send the file
try:    
    t_tx = datetime.utcnow()
    ett.add_usr_message(t_tx.strftime(tfmt))
    ett.add_usr_message(str(ch_err_prob))
    ett.add_filestore_request(CfdpFileStoreEnum.CFDP_DELETE_FILE, file)
    ett.cfdp_send(file)
    print('File sent.')
except:
	# Must have except clause to use ``else``
	raise
else:
    # Wait for the transaction to finish
    ett.wait_for_transaction_end()
    t_rx = datetime.utcnow()
    print('CHANNEL ERROR PROBABILITY:', ch_err_prob)
    print('FILE TRANSMISSION TIME:', t_tx)
    print('FILE RECEPTION TIME:', t_rx)
    print('TOTAL TRANSMISSION TIME:', t_rx-t_tx)
finally:
    # Close entitity and endpoint
    cpxy.cfdp_close(peer_nbr)
    bpxy.bp_close(ept)

    # Clear network rules
    #npxy.delete_rules()

# =================================================================
# === EOF
# =================================================================
