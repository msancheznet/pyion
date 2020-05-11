#!/usr/bin/python3

"""
# Send Python data directly over LTP
#
# Author: Marc Sanchez Net
# Date:   05/30/2019
# Copyright 2019 (c), Jet Propulsion Laboratory
"""

# Fix path (ONLY FOR THIS TEST)
import sys
sys.path.append('..')
sys.path.append('../pyion')

# General imports
import json
import time

# Import module
import pyion
import pynetem as netem

# =================================================================
# === Define global variables
# =================================================================

# Set environment variables for ION
pyion.ION_NODE_LIST_DIR = './nodes'

# ION node numbers
node_nbr, peer_nbr = 1, 2
#node_nbr, peer_nbr = 2, 1

# Client identifier
client_id = 4

# Create a proxy to LTP
pxy = pyion.get_ltp_proxy(node_nbr)

# =================================================================
# === Configure network
# =================================================================

p = netem.Proxy('eth0')
p.reset()
p.add_random_loss_rule(0.12)
p.commit_rules()
p.show_rules()

# =================================================================
# === Get image to transmit
# =================================================================

# Read file
with open('./nodes/1/data/jupiter.txt', 'r') as f:
    img = f.read()
    
# Split image in lines and create dict {line nbr: bytes in line}
lines = img.split('\n')
lines = dict(zip(range(len(lines)), lines))

# =================================================================
# === MAIN
# =================================================================

# Attach to ION's LTP engine
pxy.ltp_attach()

# Open a endpoint access point for this client and send data
with pxy.ltp_open(client_id) as sap:
    for item in lines.items():
        print('Sending line', item[0])
        sap.ltp_send(peer_nbr, json.dumps(item))       
        
# =================================================================
# === EOF
# =================================================================
