Interface with the Bundle Protocol (BP)
=======================================

**pyion** provides two main abstractions to send/receive data through BP:

- **BpProxy**: Proxy to ION engine for a given node (identified by a node number). You should only ever have one proxy per node number. To ensure that this is the case, do not create Proxies manually. Instead, use ``pyion.get_bp_proxy``.
- **Endpoint**: Endpoint to send/receive data using BP. Do not open/close/interrupt/delete endpoints manually, always do it through the proxy (note that all ``Endpoints`` reference the ``Proxy`` that create them).

While sending data through ION's BP, several properties can be specified (e.g., time-to-live, required reports, reporting endpoint, etc). These can be defined as inherent to the endpoint (i.e., all bundles send through this endpoint will have a give TTL), in which case they must be specified while calling ``bp_open`` in the ``BpProxy`` object, or as one-of properties for a specific bundle (in which case they must be specified while calling ``bp_send`` in the ``Endpoint`` object).

Not all features available in ION are currently supported. For instance, while you can request BP reports and provide a reporting endpoint, reception of the reports is for now left in binary format and it is thus up to the user to decode them. Similarly, bundles cannot be specify advanced class of service properties (ancillary data). Finally, ``pyion`` does not provide any flow control mechanisms when sending data over an endpoint. This means that if you overflow the SDR memory, a Python ``MemoryError`` exception will be raise and it is up to the user to handle it.