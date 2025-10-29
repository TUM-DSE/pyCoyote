######################################################################################
# This file is part of pyCoyote <https://github.com/fpgasystems/pyCoyote>
# 
# MIT Licence
# Copyright (c) 2025 Systems Group, ETH Zurich
# All rights reserved.
# 
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
######################################################################################

cmake_minimum_required(VERSION 3.5)

# Dependency paths
set(COYOTE_DIR ${CMAKE_SOURCE_DIR}/deps/Coyote/)
set(PYBIND_DIR ${CMAKE_SOURCE_DIR}/deps/pybind11/)

# Include Coyote's CMake modules (which builds Coyote as a shared library)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${COYOTE_DIR}/cmake)
find_package(CoyoteSW REQUIRED)

# Include pybind11
add_subdirectory(${PYBIND_DIR} pybind11)    

# Define project properties
project(
    ${SKBUILD_PROJECT_NAME}
    VERSION ${SKBUILD_PROJECT_VERSION}
    LANGUAGES CXX
)

# Add Python modules
# NOTE: The module name starts with an underscore (_pycoyote), making it a "private" module
# Then, all the Coyote Python modules are exposed as part of the pycoyote library via the main Python file, pyCoyote/__init__.py 
pybind11_add_module(_pycoyote pycoyote/cpp/cRcnfgWrapper.cpp)

# Include headers from Coyote and pybind11
set(COYOTE_INCLUDE_PATH ${COYOTE_DIR}/sw/include)
target_include_directories(_pycoyote PRIVATE ${COYOTE_INCLUDE_PATH})

set(PYBIND_INCLUDE_PATH ${PYBIND_DIR}/include)
target_include_directories(_pycoyote PRIVATE ${PYBIND_INCLUDE_PATH})

# Libraries to link against
target_link_libraries(_pycoyote PRIVATE Coyote)
target_link_directories(_pycoyote PRIVATE /usr/local/lib)

# Make sure _pycoyote can find libCoyote.so at runtime
set_target_properties(_pycoyote PROPERTIES
    BUILD_WITH_INSTALL_RPATH TRUE
    INSTALL_RPATH "$ORIGIN"
)

# Install the bindings and Coyote as a shared library
install(TARGETS _pycoyote Coyote LIBRARY DESTINATION pycoyote)