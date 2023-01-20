Interface with the CCSDS File Delivery Protocol (CFDP)
======================================================

**pyion** provides two main abstraction to send/request files through CFDP:

- **CfdpProxy**: Proxy to CFDP engine of ION. You should always obtain these type of proxies by calling ``pyion.get_cfdp_proxy``.
- **Entity**: Entity that you can use to send/request files. You can also register event handlers to for specific CFDP events. Entities can also be used to send commands to remote engines (e.g. delete/rename file, wait for a file).

Event handlers are defined in pyion as functions that must be registered through the Entity object (see example below). You can define different handlers for specific events, or a single handler for all events. In either case, the expected function signature for the event handler is

.. code-block:: python
    :linenos:

    def ev_handler(ev_type, ev_params):
      pass

``ev_type`` indicates the type of event that is being processed. In turn, ``ev_params`` is a dictionary with the parameters inherent to this event type. You can find their definition in pyion's source code (see ``_cfdp.c`` file), or in the CFDP Blue Book available at https://public.ccsds.org/Publications/BlueBooks.aspx. Finally, for convenience, the list of available CFDP events is provided in the ``constants`` module within pyion.

Finally, and assuming that CFDP transactions are performed one at a time, the Entity object provides a convenience method ``wait_for_transaction_end`` that blocks the current thread of execution until the receiver has obtained confirmation that the CFDP transaction was successful (or not). This waiting mechanism **only** works if one transaction is active at any point in time. Otherwise, there is no easy way to differentiate which of *N* concurrent transactions finished.

CFDP Example: Transmitter
-------------------------

.. code-block:: python
    :linenos:

    import pyion
    import pyion.constants as cst

    # Create a proxy to node 1 and attach to ION
    bpxy = pyion.get_bp_proxy(1)
    cpxy = pyion.get_cfdp_proxy(1)

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

CFDP Example: Receiver
----------------------

.. code-block:: python
    :linenos:
    
    import pyion
    import pyion.constants as cst

    # Create a proxy to node 2 and attach to ION
    bpxy = pyion.get_bp_proxy(2)
    cpxy = pyion.get_cfdp_proxy(2)

    # Create endpoint and entity
    ept = bpxy.bp_open('ipn:2.1')
    ett = cpxy.cfdp_open(1, ept)

    # Define handler and register it
    def ev_handler(ev_type, ev_params):
      print(ev_type, ev_params)
    ett.register_event_handler(cst.CFDP_ALL_EVENTS, cfdp_event_handler)

    # Wait for end of transaction
    ett.wait_for_transaction_end()