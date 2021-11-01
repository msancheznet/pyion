#!/usr/bin/python3

"""
# This scripts is part of the ION hardening effort. It performs the following actions:
# - Create a random file of given size
# - Compute its SHA256 hash
# - For each network condition, run the following experiment:
#   - Add metadata to the file (tx. time, network condition, file hash).
#   - Send file using CFDP
#   - Record time at which transmitter thinks file is sent.
#   - Wait for receiver to acknowledge file receipt.
#   - Repeat
#
# Author: Marc Sanchez Net
# Date:   05/20/2019
# Copyright 2019 (c), Jet Propulsion Laboratory
"""

# General imports
from collections import defaultdict
from datetime import datetime
import hashlib
import json
import numpy as np
import pandas as pd
from pathlib import Path
import string
from threading import Event, Thread
from time import sleep

# Network emulation imports
import pynetem as netem

# Pyion imports
import pyion
from pyion import BpCustodyEnum, BpPriorityEnum, BpReportsEnum
from pyion import CfdpMode

# ===================================================================================
# === Define global variables
# ===================================================================================

# Set environment variables for ION
pyion.ION_NODE_LIST_DIR = './nodes'

# Define file size to send  [bytes]
file_name = './data/cfdp_test_file.txt'
file_size = 1000000

# ION node numbers
node_nbr = 1
peer_nbr = 2

# Define endpoints
orig_eid = 'ipn:1.1'    # Endpoint used by this node's CFDP engine
dest_eid = 'ipn:2.1'    # Endpoint used by peer node CFDP engine
rept_eid = 'ipn:1.2'    # Endpoint used by this node to rx confirmations

# Define properties for all endpoints
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
ch_delay    = 1000
ch_jitter   = 250
ch_err_prob = np.arange(0, 0.91, 0.01)

# ===================================================================================
# === Create proxies for tests
# ===================================================================================

# Create a proxy to Netem
npxy = netem.Proxy('eth0')

# Create a proxies to ION
bpxy = pyion.get_bp_proxy(node_nbr)
cpxy = pyion.get_cfdp_proxy(node_nbr)

# ===================================================================================
# === Open CFDP engine and BP endpoints
# ===================================================================================

# Open CFDP entity and its endpoint
ett = cpxy.cfdp_open(peer_nbr, bpxy.bp_open(orig_eid, **ept_props), 
                     mode=CfdpMode.CFDP_CL_RELIABLE)

# Open endpoint for reporting
ept = bpxy.bp_open(rept_eid, **ept_props)

# ===================================================================================
# === Define functions for experiment
# ===================================================================================

# Initialize variables
results = defaultdict(dict)

def new_file(size=file_size):
    chars    = list(string.ascii_lowercase)
    bytes_ch = 4
    num_ch   = int(file_size/bytes_ch)
    txt = np.random.choice(chars,  size=(num_ch,)).astype('|S1')
    return txt.tostring()

def get_file_hash(txt):
    m = hashlib.sha256()
    m.update(bytes(txt))
    return m.hexdigest()

def start_CFDP_transaction(file, file_hash, per, is_last):
    # Initialize variables
    t_tx = datetime.utcnow()

    # Save start time
    results[per]['tx_start'] = t_tx

    # Construct metadata object
    mtd = {'file': './nodes/{}/data/'.format(peer_nbr)+Path(file_name).name,
           'hash': file_hash,
           'per': per,
           'is_last': is_last}

    # Send the file with the metadata
    ett.add_usr_message(json.dumps(mtd))
    ett.cfdp_send(file)

def monitor_transactions(event, results):
    while True:
        # Get the next bundle
        data = json.loads(ept.bp_receive().decode('utf-8'))

        # Store the time recorded at the receiver
        results[data['per']]['rx_end'] = data['rx_end']
        results[data['per']]['ok'] = data['ok']

        # If signal is exit, return. Otherwise, the current experiment has ended
        # but there are more to come
        if data['is_last']:
            event.set()
            break

        # Awake all threads that were waiting and reset event
        print('Report endpoint:', data)
        event.set()
        event.clear()

# ===================================================================================
# === Create thread to monitor CFDP transactions
# ===================================================================================

# Event for transaction end (signaled by receiver)
tx_end = Event()

# Start monitoring thread
th = Thread(target=monitor_transactions, args=(tx_end, results), daemon=True)
th.start()

# ===================================================================================
# === RUN EXPERIMENT
# ===================================================================================

# Get message and hash for the experiment
message = new_file()
m_hash  = get_file_hash(message)

# Save file in disk
with open(file_name, 'wb') as f:
    f.write(message)

# Iterate over all channel probabilities that you want to test
for i, per in enumerate(ch_err_prob, start=1):
    print('{}.a) Starting experiment with PER={:.2e}'.format(i,per))

    # Set channel properties
    npxy.reset()
    npxy.add_delay_rule(delay=ch_delay, jitter=ch_jitter, units='ms')
    npxy.add_random_loss_rule(error_prob=per)
    npxy.commit_rules()

    # Send file using CFDP
    start_CFDP_transaction(file_name, m_hash, per, i==len(ch_err_prob))

    # Wait until transmitter thinks file is sent
    print('{}.b) Awaiting for tx to signal end of transaction...'.format(i))
    ett.wait_for_transaction_end()
    results[per]['tx_end'] = datetime.utcnow()

    # Wait until the receiver thinks the file is sent
    print('{}.c) Awaiting for rx to signal end of transaction...'.format(i))
    tx_end.wait()

    # Wait for a little bit
    sleep(1)

# Save results and close
df = pd.DataFrame.from_dict(results, orient='index')
df.index.name = 'per'
df.to_excel('results.xlsx')

# Wait for monitor thread to end
th.join()