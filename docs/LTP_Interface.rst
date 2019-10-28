Interface with the Lickelider Transmission Protocol (LTP)
=========================================================

**pyion** provides two main abstraction to send/request data through LTP (without BP):

- **LtpProxy**: Proxy to ION's LTP engine for a given node (identified by a client id). You should only ever have one proxy per node number. To ensure that this is the case, do not create Proxies manually. Instead, use ``pyion.get_ltp_proxy``.
- **AccessPoint**: Access point to send/receive data using LTP. Do not open/close/interrupt/delete access points manually, always do it through the proxy (note that all ``AccessPoints`` reference the ``Proxy`` that create them).