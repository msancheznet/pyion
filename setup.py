# ========================================================================================
# Install pyion together with its C Extensions
#
# ..Warning:: pyion has only been tested for Python 3.5+
#
# To compile pyion
# ----------------
# root@mnet:/pyion# python3 setup.py install
# root@mnet:/pyion# python3 setup.py clean --all install # Clean before building
# root@mnet:/pyion# python3 setup.py build_ext --inplace # Build in place for testing
#
# Pyion Compilation Options
# -------------------------
# To include administrative functionality in pyion, you need to compile it against
# ION's private interface. To do that, you need to specify an environment variable
# in your system named ``ION_HOME`` that points to the home directory where ION is
# installed. If ``ION_HOME`` is not provided, all but the ``admin`` module in pyion
# will be installed.
#   
# Author:   Marc Sanchez Net
# Date:     04/12/2019
# Updated:  MSN -- 08/28/2019: Adds _admin extension
#           MSN -- 10/28/2019: Adds ability to work with LD_LIBRARY_PATH
#
# License Terms
# -------------
#
# Copyright (c) 2019, California Institute of Technology ("Caltech").  
# U.S. Government sponsorship acknowledged.
#
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without modification, 
# are permitted provided that the following conditions are met:
#
# * Redistributions of source code must retain the above copyright notice, 
#   this list of conditions and the following disclaimer.
# * Redistributions must reproduce the above copyright notice, this list 
#   of conditions and the following disclaimer in the documentation and/or other 
#   materials provided with the distribution.
# * Neither the name of Caltech nor its operating division, the Jet Propulsion Laboratory, 
#   nor the names of its contributors may be used to endorse or promote products 
#   derived from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND 
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED 
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. 
# IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, 
# INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, 
# BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
# OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, 
# WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY 
# OF SUCH DAMAGE.
# ========================================================================================

# Generic imports
import math
import os
from pathlib import Path
from setuptools import setup, Extension
import sys
from warnings import warn

__version__ = 'pyion-4.0.0'
__release__ = 'R2020a'

# ========================================================================================
# ===  Helper definitions
# ========================================================================================

class SetupWarning(UserWarning):
    pass

# The text of the README file
README = (Path(__file__).parent / "README.md").read_text()

# ========================================================================================
# === Locate paths for ION's public/private APIs
# ========================================================================================

# Grab environment variables
lib_path = os.environ.get('LD_LIBRARY_PATH')
ion_path = os.environ.get('ION_HOME')

# ION_HOME is now mandatory since bputa should be public API but is not.
if not ion_path:
    raise ValueError(("Environment variable ION_HOME must indicate the location of ION's ",
                      "source code directory"))
                       

# Set paths for ION's public API
if lib_path is None:
    # Default paths
    ion_inc = Path('/usr/local/include')    # ION headers (.h)
    ion_lib = Path('/usr/local/lib')        # ION shared libraries (.so)
else:
    lib_path = Path(lib_path)
    if not lib_path.exists():
        raise EnvironmentError('Cannot locate ION public API')
    
    # Get paths
    ion_inc = (lib_path.parent)/'include'   # ION headers (.h)
    ion_lib = lib_path                      # ION shared libraries (.so)

# Set paths for compiling _admin
ion_path  = Path(ion_path)
bp_lib   = ion_path/'bpv7'/'library'
cfdp_lib = ion_path/'cfdp'/'library'
ltp_lib  = ion_path/'ltp'/'library'
cfdp_inc = ion_path/'cfdp'/'include'
ici_inc  = ion_path/'ici'/'include'

# ========================================================================================
# === Figure out compile-time options
# ========================================================================================

# Figure out the type of machine this is being installed in
hw_type = int(math.log2(sys.maxsize))+1
if hw_type == 64:
    ds = 3
elif hw_type == 31:
    ds = 2
elif hw_type == 16:
    ds = 1
else:
    raise ValueError('Cannot determine host type (16 vs. 32 vs. 64 bits)')

# Defined C compilation options for the extensions.
compile_args = [
    '-g',
    '-Wall',
    '-O0',
    '-Wl,--no-undefined',
    '-Wno-undef',
    '-DSPACE_ORDER={}'.format(ds), 
    '-fPIC', 
    '-Wno-unused-function', 
    '-Wno-strict-prototypes',
    '-Wno-discarded-qualifiers', 
    '-Wno-unused-variable'
]

# ========================================================================================
# === Define all pyion C Extensions
# ========================================================================================

# Define ION administrative extension
_admin = Extension('_admin',
                include_dirs=[str(ion_inc), str(bp_lib), str(ltp_lib), str(cfdp_lib), str(ici_inc)],
                libraries=['ici', 'bp', 'ltp', 'cfdp'],
                library_dirs=[str(ion_lib)],
                sources=['./pyion/_admin.c'],
                extra_compile_args=compile_args
                )

# Define the ION-BP extension and related directories
_bp = Extension('_bp',
                include_dirs=[str(ion_inc)],
                libraries=['bp', 'ici','ltp', 'cfdp'],
                library_dirs=[str(ion_lib)],
                sources=['./pyion/_bp.c',
                './pyion/_utils.c',
                './pyion/base_bp.c'],
                extra_compile_args=compile_args
                )

# Define the ION-CFDP extension and related directories
_cfdp = Extension('_cfdp',
                include_dirs=[str(ion_inc), str(cfdp_inc)],    
                libraries=['cfdp', 'ici'],
                library_dirs=[str(ion_lib), str(cfdp_lib)],
                sources=['./pyion/_cfdp.c',
                './pyion/base_cfdp.c'],
                extra_compile_args=compile_args
                )

# Define the ION-LTP extension and related directories
_ltp = Extension('_ltp',
                include_dirs=[str(ion_inc)],
                libraries=['ltp', 'ici','bp','cfdp'],
                library_dirs=[str(ion_lib)],
                sources=['./pyion/_ltp.c',
                './pyion/_utils.c',
                './pyion/base_ltp.c'],
                extra_compile_args=compile_args
                )

# Define the ION memory extension and related directories
_mem = Extension('_mem',
                include_dirs=[str(ion_inc)],
                libraries=['ici', 'bp', 'cfdp', 'ltp'],        # bp is required
                library_dirs=[str(ion_lib)],
                sources=['./pyion/_mem.c'
                ],
                extra_compile_args=compile_args
                )

# Define the extensions to compile
_ext_modules = [_bp, _cfdp, _ltp, _mem]
if ion_path: _ext_modules.append(_admin)

# ========================================================================================
# === Main setup
# ========================================================================================

setup(
    name             = 'pyion',
    version          = __version__,
    release          = __release__,
    description      = 'Interface between ION and Python',
    long_description = README,
    author           = 'Marc Sanchez Net',
    author_email     = 'marc.sanchez.net@jpl.nasa.gov',
    license          = 'Apache 2.0',
    url              = 'https://github.com/msancheznet/pyion',
    python_requires  = '>=3.5.0',
    setup_requires   = [],
    install_requires = ['setuptools'],
    extras_require   = {
                'test': [],
                'docs': ['sphinx', 'sphinx_bootstrap_theme'],
            },
    entry_points     = {
                'sphinx.html_themes': [
                    'bootstrap = sphinx_bootstrap_theme',
                ]
    },
    packages         = ["pyion"],
    ext_modules      = _ext_modules
)