ION Management Functions
========================

``Pyion`` offers a basic set of management functions, mostly in the context of the Bundle Protocol and Contact Graph Routing. Additional management functions can be developed on an ad-hoc basis as new features are requested, but they are not intended to replace the management functions being natively supported by ION (e.g., ``ionadmin``, ``bpadmin``, etc.). Furthermore, the maturity of these management functions is low.

The list of functions provided to interact with CGR are:

- List contacts and ranges currently defined in ION's contact plan.
- Add/delete a contact and/or range from ION's contact plan.

The list of functions provided to interact with BP are:

- Add a new endpoint.
- Check if a given endpoint exists.
- List all known endpoints.

The list of functions provided to interact with LTP are:
- Check if an LTP span exists.
- (Not fully implemented) Obtain information about an LTP span.
- (Not fully implemented) Update the configuration of an LTP Span

The list of functions provided to interact with CFDP are:

- (Not fully implemented) Update the CFDP engine maximum PDU size.