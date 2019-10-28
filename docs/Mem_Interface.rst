Interface with ION's Memory/Storage
===================================

ION's memory is divided in two parts, the Personal Space Management (PSM) and the Simple Data Recorder (SDR). The state of both the PSM and the SDR can be directly queried from pyion through the PsmProxy and SdrProxy, both of which are still experimental at this point. The following example demonstrate how to use both while sending bundles through an endpoint:

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