#!/usr/bin/python3

"""
# Opens endpoint 'ipn:2.1' and receives data until user issues SIGINT
# or 60 seconds elapse without receiving anything.
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

# ION node number
node_nbr = 2

# Endpoint to listen to
EID = 'ipn:2.1'

# =================================================================
# === MAIN
# =================================================================

# Create a proxy to ION's BP
proxy = pyion.get_bp_proxy(node_nbr)

# Attach to ION
proxy.bp_attach()

# Open a proxy to receive data
with proxy.bp_open(EID) as eid:
    # You are now ready to received
    print('{} ready to receive'.format(eid))
    
    nbnd, nbytes, elapsed = 0, 0, 0

    # Receive
    while eid.is_open:
        try:
            # This is a blocking call. Timeout is optional
            data = eid.bp_receive(timeout=60)

            # If data is an exception, then return
            if isinstance(data, Exception):
                print('{}: Bundle reception timed-out.'.format(data.__class__.__name__))
                break

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
                print('{}) Total bytes {} / {:.3e} seconds = {:.3} bps'.format(nbnd, nbytes, elapsed, 8*nbytes/elapsed))
            except UnicodeDecodeError:
                print(data)
        except InterruptedError:
            break

