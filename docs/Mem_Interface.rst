Interface with ION's Memory/Storage
===================================

ION's memory is divided in two parts, the Personal Space Management (PSM) and the Simple Data Recorder (SDR). The state of both the PSM and the SDR can be directly queried from pyion through the PsmProxy and SdrProxy, both of which are still experimental at this point. 

Utilization of the PSM or SDR proxies works in a similar manner. The user starts a monitoring session during which the state of the system will be quiered at regular intervals as defined by the sampling rate (in Hz). Once the user has collected all necessary information, then both proxies must call ``stop_monitoring``, a function that returns a dictionary of dictionaries, where the outer layer corresponds to the variable being monitored (e.g., ``small_pool``, ``large_pool``, ``summary``, etc.) and the inner layer is a time index from the start of the monitoring sessions. This data can be then processed in tabular format using well-known data science libraries such as Pandas.

Example: Monitoring Session at Transmitter
------------------------------------------

.. code-block:: python
    :linenos:

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

