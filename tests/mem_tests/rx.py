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

from datetime import datetime
import sys
import time

# Import module
import pyion

# =================================================================
# === Define global variables
# =================================================================

# Set environment variables for ION
pyion.ION_NODE_LIST_DIR = './nodes'

# ION node number
node_nbr = 2

# Endpoint to listen to
EID = 'ipn:2.1'

# Create a proxy to ION's BP
proxy = pyion.get_bp_proxy(node_nbr)

# Attach to ION
proxy.bp_attach()

# =================================================================
# === MAIN
# =================================================================

# Open a proxy to receive data
with proxy.bp_open(EID) as eid:
    # You are now ready to received
    print('{} ready to receive'.format(eid))
    
    nbnd, nbytes, elapsed = 0, 0, 0

    # Receive
    while eid.is_open:
        try:
            # This is a blocking call
            data = eid.bp_receive()

            try:
                data = data.decode('utf-8')
                
                if nbnd == 0: tic = time.time()
                
                # Separate timestamp and data
                ts, msg = data.split(' - ')
                
                # Convert timestamp
                ts = datetime.strptime(ts, '%Y-%m-%d %H:%M:%S.%f')
                
                # Get the time it took for data to arrive
                dt = (datetime.now()-ts).total_seconds()
                
                # Report statistics
                nbnd += 1
                nbytes += sys.getsizeof(data)
                elapsed = (time.time() - tic)
                print('{}) Total bytes {} / {} seconds = {} bytes/sec'.format(nbnd, nbytes, elapsed, nbytes/elapsed))
            except UnicodeDecodeError:
                print(data)
        except InterruptedError:
            break

