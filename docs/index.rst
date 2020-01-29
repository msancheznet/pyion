.. DtnSim documentation master file, created by
   sphinx-quickstart on Thu May  3 15:24:01 2018.
   You can adapt this file completely to your liking, but it should at least
   contain the root `toctree` directive.

pyion: A Python Extension for the Interplanetary Overlay Network
================================================================

.. Above is the document title, and below is the subtitle.
   They are transformed from section titles after parsing.

.. bibliographic fields (which also require a transform):

:Author: Marc Sanchez Net
:Address: 4800 Oak Grove Dr.
		  Pasadena, CA 91109
:Contact: marc.sanchez.net@jpl.nasa.gov
:Organization: Jet Propulsion Laboratory (JPL)
:Repository: https://github.com/msancheznet/
:Release: |release|

:Abstract:
	``pyion`` is provides a set of Python C extensions that interface with the Interplanetary Overlay Network (ION), JPL's implementation of the Delay Tolerant Networking (DTN) protocol stack.

.. meta::
   :keywords: Delay Tolerant Networking

Introduction
============

``pyion`` provides a Python interface to JPL's Interplantary Overlay Network (ION) to send/receive data through ION, as well as a limited and still experimental set of administrative functions to modify the ION configuration during runtime. Note that pyion does install ION during its setup process. Instead, installation of pyion is only possible if ION is already installed and running in the host.

To quickly demonstrate how ``pyion`` works, here is a brief example of two Python nodes exchanging data by interfacing with ION's implementation of the Bundle Protocol (BP).

**Example 1: BP Transmitter**

.. code-block:: python
    :linenos:
    
    import pyion

    # Create a proxy to node 1 and attach to ION
    proxy = pyion.get_bp_proxy(1)
    proxy.bp_attach()

    # Open endpoint 'ipn:1.1' and send data to 'ipn:2.1'
    with proxy.bp_open('ipn:1.1') as eid:
        eid.bp_send('ipn:2.1', b'hello')

**Example 2: BP Receiver**

.. code-block:: python
    :linenos:

    import pyion

    # Create a proxy to node 2 and attach to it
    proxy = pyion.get_bp_proxy(2) 
    proxy.bp_attach()

    # Listen to 'ipn:2.1' for incoming data
    with proxy.bp_open('ipn:2.1') as eid:
        while eid.is_open:
            try:
                # This is a blocking call. 
                print('Received:', eid.bp_receive())
            except InterruptedError:
                # User has triggered interruption with Ctrl+C
                break


Pyion interfaces with ION through a collection of **proxies**. Each proxy is intended to be linked to a protocol in the DTN protocol stack (i.e., there is a proxy to the BP protocol, as seen in the previous example, but there are different proxies to LTP and CFDP, as well ION's SDR and PSM), as well as a node number. To avoid having muliple proxies to the same node, proxies should not be created directly by the user but rather instantiated using ``pyion.get_<type>_proxy(<node_number>)``. Proxies are used to manage service access points to a given protocol (e.g., endpoint for the BP, entity for CFDP, etc.). This includes opening and closing them, as well as interrupting their operation if necessary. In turn, the access point itself is typically only used to send and receive data.

Multiple ION Nodes in a Single Host
===================================

If multiple instances of ION are running on the same host, each one with its own node number, then ``ION_NODE_LIST_DIR`` environment variable needs to be defined (see the ION manual for further details). This can be done from Python by simply calling

.. code-block:: python
    :linenos:
    
    import pyion
    pyion.ION_NODE_LIST_DIR = '/<desired path>/nodes'

Copyright and Licensing
=======================

Copyright (c) 2019, California Institute of Technology ("Caltech").  U.S. Government sponsorship acknowledged.

``pyion`` is currently an open-source package distributed under the Apache 2.0 license.

Disclaimers
-----------

``Pyion`` is an experimental package developed in an ad-hoc basis as projects identify new needs. Basic functionality such as sending/receiving from through BP, LTP or CFDP has been tested somewhat exhaustively and thus should be stable. Newer functionality such as monitoring the state of ION's SDR and PSM, or performing administrative functions (e.g., modify the contact plan, change an LTP span configuration parameter) are still being developed and might contain errors or simply not work.

Contents
========

.. toctree::
   :maxdepth: 3
   :numbered:

   Install.rst
   BP_Interface.rst
   LTP_Interface.rst
   CFDP_Interface.rst
   Admin_Interface.rst
   Mem_Interface.rst
   Reference_Guide.rst
   License.rst


Indices and tables
==================

* :ref:`genindex`
* :ref:`modindex`
* :ref:`search`
