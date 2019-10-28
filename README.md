Introduction
============

``pyion`` provides a Python interface to JPL's Interplantary Overlay Network (ION) to send/receive data through ION, as well as a limited and still experimental set of administrative functions to modify the ION configuration during runtime. Note that pyion does install ION during its setup process. Instead, installation of pyion is only possible if ION is already installed and running in the host.

To quickly demonstrate how ``pyion`` works, we now provide a brief example of two Python nodes sending and receiving data using the ION's implementation of the Bundle Protocol (BP).

Example 1: BP Transmitter
-------------------------
```python
import pyion

# Create a proxy to node 1 and attach to ION
proxy = pyion.get_bp_proxy(1)
proxy.bp_attach()

# Open endpoint 'ipn:1.1' and send data to 'ipn:2.1'
with proxy.bp_open('ipn:1.1') as eid:
    eid.bp_send('ipn:2.1', b'hello')
```

Example 2: BP Receiver
----------------------
```python
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
```

Pyion interfaces with ION through a collection of **proxies**. Each proxy is intended to be linked to a protocol in the DTN protocol stack (i.e., there is a proxy to the BP protocol, as seen in the previous example, but there are different proxies to LTP and CFDP, as well ION's SDR and PSM), as well as a node number. Therefore, proxies should not be created by the user, but rather acquired using ``pyion.get_xxx_proxy(<node_number>)``. Proxies are used to manage service access points to a given protocol (e.g., endpoint for BP, entity for CFDP). This includes opening and closing them, as well as interrupting their operation if necessary. In turn, the access point itself is typically used to send and receive data.

Multiple ION Nodes in a Single Host
-----------------------------------
If multiple instances of ION are running on the same host, each one with its own node number, then ``ION_NODE_LIST_DIR`` environment variable needs to be defined (see the ION manual for further details). This can be done from Python by simply calling
```python
import pyion

# Set ION environment variable
pyion.ION_NODE_LIST_DIR = '/<desired path>/nodes'
```

Disclaimers
-----------

``Pyion`` is an experimental package developed in an ad-hoc basis as projects identify new needs. Basic functionality such as sending/receiving from through BP, LTP or CFDP has been tested somewhat exhaustively and thus should be stable. Newer functionality such as monitoring the state of ION's SDR and PSM, or performing administrative functions (e.g., modify the contact plan, change an LTP span configuration parameter) are still being developed and might contain errors or simply not work.

Copyright and Licensing
-----------------------

Copyright (c) 2019, California Institute of Technology ("Caltech").  U.S. Government sponsorship acknowledged.

``Pyion`` is currently an open-source package distributed under the Apache 2.0 license.

Installation
============

``Pyion`` is currently not hosted in any Python repository (e.g., pip, conda). However, installing it is as simple as downloading the package and then, in a terminal, running
```
$ cd /pyion
$ export ION_HOME=/......
$ python3 setup.py install
```

Dependencies
------------

The only dependency ``pyion`` uses is ION itself. All tests conducted to date have been performed using ION-3.6.2 over an Ubuntu-based operating system running in either a laptop, a Docker container, or a Raspberry Pi (arm architecture). The package might also work in other setups (e.g., ION-3.6.0), but there is no guarantee that this is the case.

``Pyion`` is internally built as a collection of Python C Extensions wrapped in Python classes or functions. These C extensions are compiled during the package's installation process and require access to the ION's public and potentially private interfaces (a collection of ``.h`` and ``.so`` files in an Ubuntu-based host). Therefore, the file ``setup.py`` found in the root folder of the makes assumptions on where these are located in the host file system:

- Public ION Interface: Required for installing ``pyion``. It is assumed to be located at ``/usr/local/include`` and ``usr/local/lib``. This is the default location for Ubuntu-based operating systems unless the ``LD_LIBRARY_PATH`` environment variable was set while installing ION. If that is the case, edit ``setup.py`` and point the variables ``ion_inc`` and ``ion_lib`` towards the appropriate locations.

- Private ION interface: Optional for installing ``pyion``. If not available, then ``pyion`` has all administrative functions disabled. To enable them, set the environment variable ``ION_HOME`` to ION's root folder and then run the setup process.

DTN Protocol Interfaces
=======================

Bundle Protocol
---------------

**pyion** provides two main abstractions to send/receive data through BP:

- **BpProxy**: Proxy to ION engine for a given node (identified by a node number). You should only ever have one proxy per node number. To ensure that this is the case, do not create Proxies manually. Instead, use ``pyion.get_bp_proxy``.
- **Endpoint**: Endpoint to send/receive data using BP. Do not open/close/interrupt/delete endpoints manually, always do it through the proxy (note that all ``Endpoints`` reference the ``Proxy`` that create them).

While sending data through ION's BP, several properties can be specified (e.g., time-to-live, required reports, reporting endpoint, etc). These can be defined as inherent to the endpoint (i.e., all bundles send through this endpoint will have a give TTL), in which case they must be specified while calling ``bp_open`` in the ``BpProxy`` object, or as one-of properties for a specific bundle (in which case they must be specified while calling ``bp_send`` in the ``Endpoint`` object).

Not all features available in ION are currently supported. For instance, while you can request BP reports and provide a reporting endpoint, reception of the reports is for now left in binary format and it is thus up to the user to decode them. Similarly, bundles cannot be specify advanced class of service properties (ancillary data). Finally, ``pyion`` does not provide any flow control mechanisms when sending data over an endpoint. This means that if you overflow the SDR memory, a Python ``MemoryError`` exception will be raise and it is up to the user to handle it.

Licklider Transmission Protocol
-------------------------------

**pyion** provides two main abstraction to send/request data through LTP (without BP):

- **LtpProxy**: Proxy to ION's LTP engine for a given node (identified by a client id). You should only ever have one proxy per node number. To ensure that this is the case, do not create Proxies manually. Instead, use ``pyion.get_ltp_proxy``.
- **AccessPoint**: Access point to send/receive data using LTP. Do not open/close/interrupt/delete access points manually, always do it through the proxy (note that all ``AccessPoints`` reference the ``Proxy`` that create them).

CCSDS File Delivery Protocol
----------------------------

**pyion** provides two main abstraction to send/request files through CFDP:

- **CfdpProxy**: Proxy to CFDP engine of ION. You should always obtain these type of proxies by calling ``pyion.get_cfdp_proxy``.
- **Entity**: Entity that you can use to send/request files. You can also register event handlers to for specific CFDP events.

Entities can also be used to send commands to remote engines (e.g. delete/rename file, wait for a file).

ION Memory Interfaces
=====================

ION's memory is divided in two parts, the Personal Space Management (PSM) and the Simple Data Recorder (SDR). The state of both the PSM and the SDR can be directly queried from pyion through the PsmProxy and SdrProxy, both of which are still experimental at this point. The following example demonstrate how to use both while sending bundles through an endpoint:
```python
import pyion

# Create a proxy to node 1 and attach to ION
proxy = pyion.get_bp_proxy(1)
proxy.bp_attach()

# Create proxy to SDR and PSM
sdr = pyion.get_sdr_proxy(1, 'node1')
psm = pyion.get_psm_proxy(1, 1)

# Start monitoring at a rate of 100 Hz.
sdr.start_monitoring(rate=100)
psm.start_monitoring(rate=100)

# Open endpoint 'ipn:1.1' and send data to 'ipn:2.1'
with proxy.bp_open('ipn:1.1') as eid:
    eid.bp_send('ipn:2.1', b'hello')

# Get results
sdr_res = sdr.stop_monitoring()
psm_res = psm.stop_monitoring()
```

ION Administration Interface
============================

``Pyion`` offers a basic set of administrative functions, mostly in the context of the Bundle Protocol and Contact Graph Routing. Their maturity is very low at this level, so no documentation is provided until further testing is conducted.

Other Examples
==============

CFDP Example: Transmitter
-------------------------
```python
import pyion
import pyion.constants as cst

# Create a proxy to node 1 and attach to ION
bpxy = pyion.get_bp_proxy(1)
cpxy = pyion.get_cfdp_proxy(1)

# Attach to ION
bpxy.bp_attach()
cpxy.cfdp_attach()

# Create endpoint and entity
ept = bpxy.bp_open('ipn:1.1')
ett = cpxy.cfdp_open(2, ept)

# Define handler and register it
def ev_handler(ev_type, ev_params):
  print(ev_type, ev_params)
ett.register_event_handler(cst.CFDP_ALL_EVENTS, ev_handler)

# Send a file
ett.cfdp_send('source_file')

# Wait for CFDP transaction to end
ok = ett.wait_for_transaction_end()
if ok:
  print('Successful file transfer')
else:
  print('Failed transaction')
```

CFDP Example: Receiver
----------------------
```python
import pyion
import pyion.constants as cst

# Create a proxy to node 2 and attach to ION
bpxy = pyion.get_bp_proxy(2)
cpxy = pyion.get_cfdp_proxy(2)

# Attach to ION
bpxy.bp_attach()
cpxy.cfdp_attach()

# Create endpoint and entity
ept = bpxy.bp_open('ipn:2.1')
ett = cpxy.cfdp_open(1, ept)

# Define handler and register it
def ev_handler(ev_type, ev_params):
  print(ev_type, ev_params)
ett.register_event_handler(cst.CFDP_ALL_EVENTS, cfdp_event_handler)

# Wait for end of transaction
ett.wait_for_transaction_end()
```