""" 
# ===========================================================================
# Module with utility functions and classes for pyion
#
# Author: Marc Sanchez Net
# Date:   04/17/2019
# Copyright (c) 2019, Jet Propulstion Laboratory
# ===========================================================================
"""

# Generic imports
from abc import ABC
from datetime import datetime, timedelta
from enum import Enum, unique
from functools import wraps
import os
from pathlib import Path
import time

import pyion

# ============================================================================
# === Define variables/methods/classes that will be directly importable
# ============================================================================

__all__ = ['check_ion_env_vars']

# ============================================================================
# === Error messages
# ============================================================================

_env_var_missing = ('{} enviornment variable is not properly set',
                    'Current value is {}',
                    'You can either set it in bash using ``export ${}`` or',
                    'inside Python using ``pyion.{} = "..."``')

_path_missing = 'Path "{}" does not exist'

# ============================================================================
# === Abstract Proxy
# ============================================================================

class Proxy(ABC):
    """ Abstract proxy to ION. 

        Public Variables
        ----------------
        :ivar node_nbr:  Node number that this proxy is action upon.
        :ivar node_dir:  Directory for this node
        :ivar attached:  True if proxy is attached to ION.
    """
    def __init__(self, node_nbr):
        # Initialize variables
        self.node_nbr = node_nbr
        self.attached = False

        # Set the ION_NODE_LIST_DIR variable if necessary. This call resolves relative
        # paths, makes sure directory exits.
        set_ion_node_list_dir(pyion.ION_NODE_LIST_DIR)

        # Get ION_NODE_LIST_DIR environment variable
        nodes_path = os.environ.get('ION_NODE_LIST_DIR')

        # Set node number and node dir
        self.node_dir = Path(nodes_path)/str(node_nbr) if nodes_path else None

    def __str__(self):
        return '<{}: {} ({})>'.format(self.__class__.__name__, self.node_nbr,
                                      'Attached' if self.attached else 'Detached')

    def __repr__(self):
        return str(self)

# ============================================================================
# === Helper functions
# ============================================================================

def in_ion_folder(func):
    """ Decorator to change directory inside a node's folder to run ION
        commands. It can only be used from within the ``Proxy`` class
    """
    @wraps(func)
    def wrapper(self, *args, **kwargs):
        # Get the node's directory
        node_dir = getattr(self, 'node_dir')

        # If no directory specified, just run. This is always the case
        # unless you run multiple ION nodes in a single machine.
        if node_dir is None: return func(self, *args, **kwargs)
    
        # Store current working directory
        cur_dir = os.getcwd()

        # Go to the node's directory
        os.chdir(str(node_dir.absolute()))

        # Execture function
        try:
            ret = func(self, *args, **kwargs)
        finally:
            # Go back to the previous working directory
            os.chdir(cur_dir)

        return ret
    return wrapper

def _chk_attached(func):
    """ Decorator that checks if Proxy is attached """
    @wraps(func)
    def wrapper(self, *args, **kwargs):
        # You need to be attached
        if not self.attached:
            raise IOError('Not attached to ION, call ``attach`` first.')

        # Call function
        return func(self, *args, **kwargs)
    return wrapper

def _chk_is_open(func):
    """ Decorator that checks if PoP is opened """
    @wraps(func)
    def wrapper(self, *args, **kwargs):
        # You need to be attached
        if not self.is_open:
            raise ConnectionAbortedError('{}'.format(self))

        # Call function
        return func(self, *args, **kwargs)
    return wrapper

def check_ion_env_vars(ION_NODE_LIST_DIR):
    """ Check the ION environment variables to ensure they are consistent (e.g., 
         the paths set are valid and exist in the host).

         Host environment variables are only utilized if the module variables
         ``pyion.ION_PWD`` and ``pyion.ION_NODE_LIST_DIR`` are None.
    """
    # Initialize variables
    ion_nld = ION_NODE_LIST_DIR

    # If not set, use environment variables
    if ion_nld is None:
        ion_nld = os.environ.get('ION_NODE_LIST_DIR')

    # Convert to valid path
    try:
        ion_nld = Path(ion_nld).resolve().absolute()
    except TypeError:
        msg = '\n'.join(_env_var_missing)
        raise OSError(msg.format('ION_NODE_LIST_DIR', ion_nld, 'ION_NODE_LIST_DIR', 'ION_NODE_LIST_DIR'))

    # If path does not exists, raise error
    if not ion_nld.exists():
        raise IOError(_path_missing.format(ion_nld))

    # Set the module variables
    return ion_nld

def set_ion_node_list_dir(path):
    """ Set the ION_NODE_LIST_DIR environment variable. Is not a valid
        OS path, an exception is raise.
        
        :param str: Relative or absolute path.
    """
    # If empty, just return
    if path is None: return

    # Check consistency of this variable
    ION_NODE_LIST_DIR = check_ion_env_vars(path)
    
    # Set it in the environment
    os.environ['ION_NODE_LIST_DIR'] = str(ION_NODE_LIST_DIR)

def _register_proxy(proxy_map, key, proxy_cls, *args, **kwargs):
    """ Implements the singleton mechanism for proxies """
    # If it already exists, return it
    if key in proxy_map: return proxy_map[key]

    # Create a new one. It gets registered during initialization
    proxy = proxy_cls(*args, **kwargs)

    # Register this proxy
    proxy_map[key] = proxy

    # Return new proxy
    return proxy

def _unregister_proxy(proxy_map, node_nbr):
    # Try to unregister node. If not possible, return
    try:
        del proxy_map[str(node_nbr)]
    except KeyError:
        pass

def rel2abs_time(str_time):
    """ Assumes that relative times are provided as ``+xxxx`` where
        xxxx is the time gap in seconds.
    """
    # If this is an absolute time, return
    if '+' not in str_time: return str_time        

    # Compute the date from now
    tstart = datetime.now() + timedelta(seconds=int(str_time[1:]))

    # Transform to ION time format
    return tstart.strftime('%Y/%m/%d-%H:%M:%S')

class Rate():
    def __init__(self, rate):
        self.rate = rate
        self.tic  = time.time()

    def sleep(self):
        # Sleep for a while
        time.sleep(max(0, 1.0/self.rate - (time.time()-self.tic)))

        # Reset tic
        self.tic = time.time()