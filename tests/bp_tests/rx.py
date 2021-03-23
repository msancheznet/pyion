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
import json
import threading

# Import module
import pyion

# =================================================================
# === Define global variables
# =================================================================

# ION node number
node_nbr = 2

# Endpoint to listen to
EID = 'ipn:2.1'

# Number of threads
num_threads = 4

# Create a proxy to ION's BP
proxy = pyion.get_bp_proxy(node_nbr)

# Open endpoint
ept = proxy.bp_open(EID)

# =================================================================
# === Receive function
# =================================================================

def rx_data(ept):
    # Initialize variables
    th_id = threading.current_thread().name
    nbnd, nbytes, elapsed = 0, 0, 0

    # You are now ready to received
    print('[{} / {}] ready to receive'.format(th_id, ept))
    
    # Receive
    while ept.is_open:
        try:
            # This is a blocking call. Timeout is optional
            data = ept.bp_receive(timeout=120)

            # If data is an exception, then return
            if isinstance(data, Exception):
                print('{}: Bundle reception timed-out.'.format(data.__class__.__name__))
                break

            try:
                data = json.loads(data)
                print(data)
                
                if nbnd == 0: tic = time.time()
                
                # Convert timestamp
                ts = datetime.strptime(data['time'], '%Y-%m-%d %H:%M:%S.%f')
                
                # Get the time it took for data to arrive
                dt = (datetime.now()-ts).total_seconds()
                
                # Report statistics
                nbnd += 1
                nbytes += sys.getsizeof(data)
                elapsed = (time.time() - tic)
                print('[{} / {}] Total bytes {} / {:.3e} seconds = {:.3} bps'.format(th_id, nbnd, nbytes, elapsed, 8*nbytes/elapsed))
            except UnicodeDecodeError:
                print(data)
        except InterruptedError:
            break

def rx_data_thread(ept):
    th = threading.Thread(target=rx_data, args=(ept,), daemon=True)
    th.start()
    return th

# =================================================================
# === MAIN
# =================================================================

# Start threads
threads = [rx_data_thread(ept) for _ in range(num_threads)]

# Join all data threads
for th in threads:
    th.join()