#!/usr/bin/python3

"""
# Receive Python data directly over LTP
#
# Author: Marc Sanchez Net
# Date:   05/30/2019
# Copyright 2019 (c), Jet Propulsion Laboratory
"""

# Fix path for this test
import sys
sys.path.append('..')
sys.path.append('../pyion')

# General imports
from copy import deepcopy
import json

# Import module
import pyion

# =================================================================
# === Define global variables
# =================================================================

# Set environment variables for ION
pyion.ION_NODE_LIST_DIR = './nodes'

# ION node numbers
node_nbr = 2
peer_nbr = 1

# Client identifier
client_id = 4

# =================================================================
# === MAIN
# =================================================================

# Create a proxies to ION
pxy = pyion.get_ltp_proxy(node_nbr)

# Attach to ION's BP and CFDP
pxy.ltp_attach()

# Initialize an empty image
img, last_img = [], []

# Open a service access point and receive data
with pxy.ltp_open(client_id) as sap:
    # You are now ready to received
    print('LTP ready to receive data')

    # Receive
    while sap.is_open:
        # This is a blocking call. Get the next line
        try:
            data = json.loads(sap.ltp_receive().decode('ascii'))
        except ConnectionAbortedError:
            break
            
        # Get the line number and data
        nbr, line = data[0], data[1]
        
        # Allocate new space if missing
        if nbr >= len(img):
            img += ['__']*(nbr-len(img)+1)
            
        # Add this line to the right spot
        try:
            img[nbr] = line
        except:
            print(img)
            print(len(img))
            print(nbr)
            
        for line in last_img:
            sys.stdout.write('\b '*len(line))
            sys.stdout.write("\033[F")
        
        # Print new image
        print('\n'.join(img))
        
        last_img = deepcopy(img)
        
        
# =================================================================
# === EOF
# =================================================================