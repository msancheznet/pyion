ION Management Functions
========================

``Pyion`` offers a basic set of management functions, mostly in the context of the Bundle Protocol and Contact Graph Routing. 
Additional management functions can be developed on an ad-hoc basis as new features are requested, but they are not intended to replace the management functions being natively supported by ION (e.g., ``ionadmin``, ``bpadmin``, etc.). 
Furthermore, the maturity of these management functions is low.

The list of functions provided to interact with CGR are:

- List contacts and ranges currently defined in ION's contact plan.
- Add/delete a contact and/or range from ION's contact plan.

**Example 1: List all contacts and ranges**

.. code-block:: python
    :linenos:
    
    import pyion
    from pprint import pprint

    pprint(pyion.cgr_list_contacts())
    pprint(pyion.cgr_list_ranges())



The list of functions provided to interact with BP are:

- Add a new endpoint.
- Check if a given endpoint exists.

The list of functions provided to interact with LTP are:

- Check if an LTP span exists.

Updates from Past Versions
--------------------------

Prior to pyion 4.1.2, the management module was know as the "admin" module. However, this change is transparent to the user
because all management functionality is accessed direction from the ``pyion`` module, as shown in Example 1.