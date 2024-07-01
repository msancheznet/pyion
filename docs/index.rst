.. pyion documentation master file, created by
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
:Release: |release|
:Repository: https://github.com/msancheznet/pyion

:Abstract:
	``pyion`` is provides a set of Python C extensions that interface with the Interplanetary Overlay Network (ION), JPL's implementation of the Delay Tolerant Networking (DTN) protocol stack.

.. meta::
   :keywords: Delay Tolerant Networking

Introduction
============

``pyion`` provides a Python interface to JPL's Interplantary Overlay Network (ION) to send/receive data through ION, as well as a limited and still experimental set of management functions to modify the ION configuration during runtime. Note that pyion does **NOT** install ION during its setup process. Instead, installation of pyion is only possible if ION is already available and running in the host.

To quickly demonstrate how ``pyion`` works, here is a brief example of two Python nodes exchanging data by interfacing with ION's implementation of the Bundle Protocol (BP).

**Example 1: BP Transmitter**

.. code-block:: python
    :linenos:
    
    import pyion

    # Create a proxy to node 1 and attach to ION
    proxy = pyion.get_bp_proxy(1)

    # Open endpoint 'ipn:1.1' and send data to 'ipn:2.1'
    with proxy.bp_open('ipn:1.1') as eid:
        eid.bp_send('ipn:2.1', b'hello')

**Example 2: BP Receiver**

.. code-block:: python
    :linenos:

    import pyion

    # Create a proxy to node 2 and attach to it
    proxy = pyion.get_bp_proxy(2) 

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
-----------------------------------

If multiple instances of ION are running on the same host, each one with its own node number, then the ``ION_NODE_LIST_DIR`` environment variable needs to be defined (see the ION manual for further details). This can be done from Python by simply calling

.. code-block:: python
    :linenos:
    
    import pyion
    pyion.ION_NODE_LIST_DIR = '/<desired path>/nodes'

Reporting Bugs
--------------

Known bugs are currently documented at https://github.com/msancheznet/pyion/issues. Please submit new issues if new problems and/or bug occur.
 
Installation Instructions
=========================

``Pyion`` is currently not hosted in any Python repository (e.g., pip, conda) because installation of the package is only possible in computers where ION is already available. 
To compile ION and pyion, several dependencies are needed. Therefore, to facilitate the installation process, here is a dockerfile that automates the building an Ubuntu-based Docker image with both ION and pyion installed in them.  
The steps shown in the file can also be used to install both programs in a host computer manually, and minimal adaptions are needed to operate in other operating systems. Also, note
that this file assumes that ION and pyion 4.1.3 are being installed. It is provided here as guidance, and the user should modify as needed.

.. code-block:: dockerfile
    :linenos:

    	# Define base image and pull it
	ARG IMAGE_NAME=ubuntu:20.04
	FROM $IMAGE_NAME
	
	# =====================================================
	# === SET WORKING DIRECTORY
	# =====================================================
	
	# Set environment variables.
	ENV HOME /home
	ENV ION_HOME /home/ion-open-source-4.1.3
	ENV PYION_HOME /home/pyion-4.1.3
	ENV PYION_BP_VERSION BPv7
	
	# Define working directory.
	WORKDIR /home
	
	# =====================================================
	# === INSTALL DEPENDENCIES
	# =====================================================
	
	# Install basic dependencies
	RUN apt update
	RUN apt install -y git
	RUN apt install -y --no-install-recommends man-db
	RUN apt install -y --no-install-recommends build-essential
	RUN apt install -y --no-install-recommends dos2unix
	  
	# Install ION adependencies
	RUN apt install -y --no-install-recommends autotools-dev
	RUN apt install -y --no-install-recommends automake
	RUN apt install -y --no-install-recommends libtool
	  
	# Install Python dependencies
	RUN apt install -y --no-install-recommends python3-dev
	RUN apt install -y --no-install-recommends python3-setuptools
	
	# Clean up (see https://www.fromlatest.io/#/ and 
	# https://hackernoon.com/tips-to-reduce-docker-image-sizes-876095da3b34)
	RUN rm -rf /var/lib/apt/lists/*
	
	# =============================================================
	# === DOWNLOAD, COMPILE AND BUILD ION
	# =============================================================
	
	RUN git clone --single-branch --branch ion-open-source-4.1.3 https://github.com/nasa-jpl/ION-DTN.git $ION_HOME
	
	RUN \
	    cd $ION_HOME && \
	    autoreconf -fi && \
	    ./configure && \
	    make && \
	    make install && \
	    ldconfig

	# =============================================================
	# === DOWNLOAD PYION FROM GITHUB AND COMPILE IT
	# =============================================================
	
	RUN git clone --single-branch --branch v4.1.3 https://github.com/msancheznet/pyion.git $PYION_HOME
	
	RUN \
	   cd $PYION_HOME && \
	   find $PYION_HOME -type f -print0 | xargs -0 dos2unix && \
	   python3 setup.py install && \
	   chmod -R +x $PYION_HOME
	
	# =====================================================
	# === OPEN BASH TERMINAL UPON START
	# =====================================================
	
	# Define default command.
	CMD ["tail", "-f", "/dev/null"]

To use this dockerfile in a host with Docker, simply run:

.. code-block:: bash
    :linenos:

    docker build -t pyion_bpv7:4.1.3 -f .\pyion_v413_bpv7.dockerfile --build-arg IMAGE_NAME=ubuntu:20.04 .


Note that to install pyion, three environment variables are used:
    - ``$ION_HOME``, which points to the base path where the ION source code is located (i.e., it contains the ION manual)
    - ``$PYION_HOME``, which points to the base path where the pyion source code is located (i.e., it contains ``setup.py``)
    - ``$PYION_BP_VERSION``. It must be set to either ``BPv6`` or ``BPv7``, depending on which version of the BP was used when compiling ION.
Additionally, the user can also specify the ``LD_LIBRARY_PATH`` environment variable to indicate the location of the ION shared libraries (.h and .so files).

Finally, to use pyion with BPv6, ION must also be configured to use BPv6. This is achieved by using ``./configure --enable-bpv6`` while compiling ION.

Dependencies
------------

The only dependency ``pyion`` uses is ION itself. All tests conducted to date have been performed using an Ubuntu-based operating system running in either a laptop, a Docker container, or a Raspberry Pi (arm architecture). The package might also work in other setups, but there is no guarantee that this is the case.

``Pyion`` is internally built as a collection of Python C Extensions wrapped in Python classes or functions. These C extensions are compiled during the package's installation process and require access to the ION's public and potentially private interfaces (a collection of ``.h`` and ``.so`` files in an Ubuntu-based host). Therefore, ``setup.py`` makes the following assumptions on where these are located in the host file system:

- Public ION Interface: Required for installing ``pyion``. It is assumed to be located at ``/usr/local/include`` and ``usr/local/lib`` unless the environment variable ``LD_LIBRARY_PATH`` is available.

- Private ION interface: Optional for installing ``pyion``. If not available, then ``pyion`` has all administrative functions disabled. To enable them, set the environment variable ``ION_HOME`` to ION's root folder and then run the setup process.

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
   :maxdepth: 4
   :numbered:

   BP_Interface.rst
   LTP_Interface.rst
   CFDP_Interface.rst
   Mem_Interface.rst
   Mgmt_Interface.rst
   Reference_Guide.rst
   License.rst

..
    Indices and tables
    ==================

    * :ref:`genindex`
    * :ref:`modindex`
    * :ref:`search`
