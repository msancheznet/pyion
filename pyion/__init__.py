""" 
# ===========================================================================
# **pyion** provides a Python interface to JPL's Interplantary Overlay Network 
# (ION) via a Proxy class that allows create Endpoints through which you can
# send or receive bytes, bytearrays and memoryviews. They operate in a 
# manner similar to how TCP/UDP sockets operate.
#
# pyion also provides similar abstractions to interface with
# - LTP
# - CFDP
# - SDR and PSM
# - Management functions such as add/delete a contact
#
# **NONE OF THE PRIMITIVES PROVIDED IN PYION ARE THREAD-SAFE**
#
# pyion does not substitute traditional ION management functions such as ``ionadmin``
# or ``bpadmin`` since only a small subset functionality is implemented. Therefore, to
# use pyion, it is required that
# (1) ION be installed in the host computer
# (2) One or multiple ION nodes are already instantiated and running.
#
# Transmitter Example
# -------------------
# >>> proxy = pyion.get_bp_proxy(1) 
# >>> proxy.bp_attach()
# >>>
# >>> with proxy.bp_open('ipn:1.1') as eid:
# >>>     eid.bp_send('ipn:2.1', 'hello')
#
# Receiver Example
# ----------------
# >>> proxy = pyion.get_bp_proxy(2) 
# >>> proxy.bp_attach()
# >>>
# >>> with proxy.bp_open('ipn:2.1') as eid:
# >>>     while eid.is_open:
# >>>         try:
# >>>             print('Received:', eid.bp_receive())
# >>>         except InterruptedError:
# >>>			  break
#
# Author: Marc Sanchez Net
# Date:   04/17/2019
# Copyright (c) 2019, California Institute of Technology ("Caltech").  
# ===========================================================================
"""

# General imports
import os    

# This pulls up import so that from the user you can do
# ``import pyion`` instead of having to do one of the following:
#   - ``from pyion import pyion``
#   - ``import pyion.pyion as pyion``
from pyion.proxies import *
from pyion.utils import *
from pyion.constants import *

# Handle case where _admin was not compiled during setup
try:
    from pyion.admin import *
except ImportError:
    pass

# Must be set if multiple ION nodes are run on the same host.
ION_NODE_LIST_DIR = None