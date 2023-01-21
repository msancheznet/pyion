BPv6 vs. BPv7
=============

ION now supports both Bundle Protocol v6 (RFC 5050) and Bundle Protocol v7 (RFC 9171), but not simultaneously. At compile time, the user specifies whether ION will be built to support BPv6 or BPv7. Much in the same manner, pyion can also support BPv6 or BPv7, but its version must match the version that ION was built with. The environment variable ``PYION_BP_VERSION`` must be set prior to installing pyion to indicate which version of BP is being used.

Interface with the Bundle Protocol (BP)
=======================================

**pyion** provides two main abstractions to send/receive data through BP:

- **BpProxy**: Proxy to ION engine for a given node (identified by a node number). You should only ever have one proxy per bundle node. To ensure that this is the case, do not create Proxies manually. Instead, use ``pyion.get_bp_proxy``.
- **Endpoint**: Endpoint to send/receive data using BP. Do not open/close/interrupt/delete endpoints manually, always do it through the proxy (note that all ``Endpoints`` reference the ``Proxy`` that created them).

While sending data through ION's BP, several properties can be specified (e.g., time-to-live, required reports, reporting endpoint, etc). These can be defined as inherent to the endpoint (i.e., all bundles send through this endpoint will have a give TTL), in which case they must be specified while calling ``bp_open`` in the ``BpProxy`` object, or as one-of properties for a specific bundle (in which case they must be specified while calling ``bp_send`` in the ``Endpoint`` object).

Not all features available in ION are currently supported. For instance, while you can request BP reports while using pyion (and provide a reporting endpoint), reception of the reports is for now left in binary format and it is thus up to the user to decode them. Similarly, bundles cannot specify advanced class of service properties (ancillary data). Finally, ``pyion`` does not provide any flow control mechanisms when sending data over an endpoint. This means that if you overflow the SDR memory, a Python ``MemoryError`` exception will be raised and it is up to the user to handle it.

Endpoints as Python Context Managers
------------------------------------

Endpoint objects can be used as context managers since they should always be openend and closed, just like file descriptors or sockets. The following example shows how to use endpoints as context managers

**Example 1.a: BP Transmitter**

.. code-block:: python
    :linenos:
    
    import pyion

    # Create a proxy to node 1 and attach to ION
    proxy = pyion.get_bp_proxy(1)

    # Open endpoint 'ipn:1.1' and send data to 'ipn:2.1'
    with proxy.bp_open('ipn:1.1') as eid:
        eid.bp_send('ipn:2.1', b'hello')

**Example 1.b: BP Receiver**

.. code-block:: python
    :linenos:

    python
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
                
Note that the endpoint is opened from the proxy. The same is true for closing and interrupting and endpoint. Therefore, the only operations that can be triggered directly from the endpoint are send and receive (some additional convencience operations are also available, e.g.,  send a file).
                
Endpoints as Class Instances
----------------------------

Endpoints can also be used as class instances that are manually opened and closed by the user. The later is actually not necessary since endpoints will be closed automatically when they are garbage collected.

Here is a more complicated example of a transmitter and receiver that exchange data. At the receiver, the amount of data exchanged is measured, as well as the data rate at which the connection is running. Additionally, this example also demonstrates that typical bundle properties can be provided to an endpoint as a way to configure the BP operation.

**Example 2.a: BP Transmitter**

.. code-block:: python
    :linenos:
    
    from datetime import datetime
    from threading import Thread
    import time

    # Import module
    import pyion
    from pyion import BpCustodyEnum, BpPriorityEnum, BpReportsEnum

    # =================================================================
    # === Define global variables
    # =================================================================

    # ION node number
    node_nbr = 1

    # Originating and destination endpoints
    orig_eid = 'ipn:1.1'
    dest_eid = 'ipn:2.1'
    rept_eid = 'ipn:1.2'

    # Define endpoint properties
    ept_props = {
        'TTL':          3600,   # [sec]
        'custody':      BpCustodyEnum.SOURCE_CUSTODY_REQUIRED,
        'priority':     BpPriorityEnum.BP_EXPEDITED_PRIORITY,
        'report_eid':   rept_eid,
        'report_flags': BpReportsEnum.BP_RECEIVED_RPT
        #'report_flags': BpReportsEnum.BP_RECEIVED_RPT | BpReportsEnum.BP_CUSTODY_RPT,
    }

    # Create a proxy to ION
    proxy = pyion.get_bp_proxy(node_nbr)

    # =================================================================
    # === Acquire reports
    # =================================================================

    # Open endpoint to get reports
    rpt_eid = proxy.bp_open(rept_eid)

    def print_reports():
        while True:
            try:
                data = rpt_eid.bp_receive()
                print(data)
            except InterruptedError:
                break
            
    # Start monitoring thread
    th = Thread(target=print_reports, daemon=True)
    th.start()

    # =================================================================
    # === MAIN
    # =================================================================

    # Open a endpoint and set its properties. Then send file
    with proxy.bp_open(orig_eid, **ept_props) as eid:
        for i in range(50):
            eid.bp_send(dest_eid, str(datetime.now()) + ' - ' + 'a'*1000)
        
    # Sleep for a while and stop the monitoring thread
    time.sleep(2)
    proxy.bp_interrupt(rept_eid)
    th.join()

**Example 2.b: BP Receiver**

.. code-block:: python
    :linenos:

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
                    print('{}) Total bytes {} / {:.3f} seconds = {:.3f} bytes/sec'.format(nbnd, nbytes, elapsed, nbytes/elapsed))
                except UnicodeDecodeError:
                    print(data)
            except InterruptedError:
                break
                
Updates from Previous Versions
------------------------------

The following is a non-comprehensive list of updates included in pyion:
- BpProxies attach to ION automatically upon creation. It is no longer needed for the user to manually call ``bp_attach``. Similarly, BpProxies detach from ION automatically upon deletion.
- If ION and pyion are run with BPv6, bundles can be sent with a retransmission timer. Use the ``retx_timer`` property of ``bp_send`` to control how often bundles should be retransmitted.
- While receiving, Endpoint objects can be given a timeout that will stop the reception process if no bundle arrives within a given timeframe.
- The size of the data sent or received via an Endpoint should not exceed 2^32-1 bits. 
