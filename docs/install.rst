Installation Instructions
=========================

``Pyion`` is currently not hosted in any Python repository (e.g., pip, conda) because installing the package is only possible in hosts where ION is already available.

Pre-Installation Steps
----------------------

1) Download ION from https://sourceforge.net/projects/ion-dtn/ making sure that you match the ION and pyion version number.
2) If necessary, copy-paste the downloaded source code into a more desirable/permanent location (pyion will need to link against this location).
3) Install ION (see installation script below).
4) Install Python3.5 (or a later version):

   - Ubuntu: No additional steps required, comes with python 3.5 by default.
   - CentOS: ``sudo yum install python3``
5) Install Python development tools:

   - Ubuntu: ``sudo apt-get install python3-dev``
   - CentOS: ``sudo yum install python3-devel``
6) Set LD_LIBRARY_PATH environment variable in ``.bashrc``:

   - Ubuntu: Not required for default value (``/usr/local/lib``). Set it for any other system path.
   - CentOS: Must be set for both the default value and any other system path.

Installation Steps
------------------

7) Create the ION_HOME environment variable (``export ION_HOME=/...``) and point it to ION's main folder (where its source code is located, see step 1).
8) Install pyion: ``sudo -E python3 setup.py install``

Summary of Dependencies
-----------------------

**NOTE: As of pyion-3.7.0, the environment variable ``ION_HOME`` is mandatory**

The only dependency ``pyion`` uses is ION itself. Different ION versions are mapped to pyion as branches in the GitHub repository (e.g., for ION 3.7.0, pull from the branch v3.7.0). ION versions for which there is no explicit branch might be work with pyion, but no compatibility guarantees are enforced. Also, all tests conducted to date have been performed over an Ubuntu or CentOS operating system running in either a laptop, a Docker container, or a Raspberry Pi (arm architecture).

``Pyion`` is internally built as a collection of Python C Extensions wrapped in Python classes or functions. These C extensions are compiled during the package's installation process and require access to the ION's public and potentially private interfaces (a collection of ``.h`` and ``.so`` files in an Ubuntu-based host). Therefore, the file ``setup.py`` found in the root folder of the makes assumptions on where these are located in the host file system:

- Public ION Interface: Required for installing ``pyion``. It is assumed to be located at ``/usr/local/include`` and ``usr/local/lib`` unless the environment variable ``LD_LIBRARY_PATH`` is available.

- Private ION interface: Optional for installing ``pyion``. If not available, then ``pyion`` has all administrative functions disabled. To enable them, set the environment variable ``ION_HOME`` to ION's root folder and then run the setup process.

ION Installation Script
-----------------------

.. role:: bash(code)
   :language: bash
   
   autoheader
   aclocal
   autoconf
   automake
   ./configure CFLAGS='-O0 -ggdb3' CPPFLAGS='-O0 -ggdb3' CXXFLAGS='-O0 -ggdb3'
   make
   make install
   ldconfig
