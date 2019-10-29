Interface with the Lickelider Transmission Protocol (LTP)
=========================================================

**pyion** provides two main abstraction to send/request data through LTP (without BP):

- **LtpProxy**: Proxy to ION's LTP engine for a given node (identified by a client id). You should only ever have one proxy per node number. To ensure that this is the case, do not create Proxies manually. Instead, use ``pyion.get_ltp_proxy``.
- **AccessPoint**: Access point to send/receive data using LTP. Do not open/close/interrupt/delete access points manually, always do it through the proxy (note that all ``AccessPoints`` reference the ``Proxy`` that create them).

Under most normal circumstances LTP will ensure delivery of data to destination without errors. However, just like TCP or any other practical Automatic Repeat Request mechansims, LTP eventually ceases transmission if it has no success in getting any bytes through (this is a defense mechanism to avoid having LTP hung forever). When that happens, ``ltp_send`` will raise a RuntimeError exception that must be processed by the user.

**Example 1: LTP Transmitter**

.. code-block:: python
    :linenos:

    # General imports
    import pickle
    import numpy as np
    import string

    # Import module
    import pyion

    # =================================================================
    # === Define global variables
    # =================================================================

    # ION node numbers
    node_nbr, peer_nbr = 1, 2

    # Client identifier
    client_id = 4

    # Define data size in bytes. Set to 1200 bytes because LTP block and 
    # LTP segment are set to this value
    data_size = 2400

    # Define data to send as a stream of random ASCII chars. Each char
    # uses 1 byte.
    chars = list(string.ascii_lowercase)
    data  = np.random.choice(chars, size=(data_size,)).astype('|S1').tostring()

    # =================================================================
    # === MAIN
    # =================================================================

    # Create a proxy to LTP
    pxy = pyion.get_ltp_proxy(node_nbr)

    # Attach to ION's LTP engine
    pxy.ltp_attach()

    # Open a endpoint access point for this client and send data
    with pxy.ltp_open(client_id) as sap:
        print('Sending', data)
        sap.ltp_send(peer_nbr, data)

**Example 1: LTP Receiver**

.. code-block:: python
    :linenos:

    # General imports
    import pickle

    # Import module
    import pyion

    # =================================================================
    # === Define global variables
    # =================================================================

    # ION node numbers
    node_nbr, peer_nbr = 2, 1

    # Client identifier
    client_id = 4

    # =================================================================
    # === MAIN
    # =================================================================

    # Create a proxies to ION
    pxy = pyion.get_ltp_proxy(node_nbr)

    # Attach to ION's BP and CFDP
    pxy.ltp_attach()

    # Open a service access point and receive data
    with pxy.ltp_open(client_id) as sap:
        # You are now ready to received
        print('LTP ready to receive data')

        # Receive
        while sap.is_open:
            try:
                # This is a blocking call
                data = sap.ltp_receive()
                print(data)
            except ConnectionAbortedError:
                break