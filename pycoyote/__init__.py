# Main __init__.py file, which allows pyCoyote to be imported as a Python library

# NOTE: From the docs, this file must be placed in <package_name>, src/<package_name>, or python/<package_name> 
# to be auto-discovered; in our case, <package_name> corresponds to pycoyote

# Expose all the Python functions, classes etc.
from ._cThread import *
from ._cRcnfg import *
