Installation Instructions
=========================

``Pyion`` is currently not hosted in any Python repository (e.g., pip, conda) because installing the package is only possible in hosts where ION is already available.

Pre-Installation Steps
----------------------

1) Download ION from https://sourceforge.net/projects/ion-dtn/ making sure that you match the ION and pyion version number.
2) Install Python3:

   - Ubuntu: No additional steps required, comes with python 3.5 by default.
   - CentOS: ``sudo yum install python3``
3) Install Python development tools:

   - Ubuntu: ``sudo apt-get install python3-dev``
   - CentOS: ``sudo yum install python3-devel``
4) Set LD_LIBRARY_PATH environment variable in ``.bashrc``:

   - Ubuntu: Not required for default value (``/usr/local/lib``). Set it for any other system path.
   - CentOS: Must be set for both the default value and any other system path.

Installation Steps
------------------

5) Create the ION_HOME environment variable (``export ION_HOME=/...``) and point it to ION's main folder (where its source code is located, see step 1).
6) Install pyion: ``python3 setup.py install``
