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

# Paths to include directories
set(PYBIND_INCLUDE_PATH ${PYBIND_DIR}/include)
set(COYOTE_INCLUDE_PATH ${COYOTE_DIR}/sw/include)
set(PYCOYOTE_INCLUDE_PATH ${CMAKE_SOURCE_DIR}/pycoyote/cpp/include/)

# Define project properties
project(
    ${SKBUILD_PROJECT_NAME}
    VERSION ${SKBUILD_PROJECT_VERSION}
    LANGUAGES CXX
)

# For GPU - FPGA P2P, EN_GPU must be defined when compiling pyCoyote
option(EN_GPU "Enable GPU support in pyCoyote" OFF)
if (DEFINED ENV{PYCOYOTE_EN_GPU})
    if ($ENV{PYCOYOTE_EN_GPU})
        set(EN_GPU ON CACHE BOOL "Enable GPU support in pyCoyote" FORCE)
        message(STATUS "GPU support enabled")
        add_compile_definitions(EN_GPU)
    endif()
endif()

# Include Coyote
add_subdirectory(${COYOTE_DIR}/sw coyote)

# Include pybind11
add_subdirectory(${PYBIND_DIR} pybind11)    

# Define pyCoyoteLibs as a shared library
add_library(pyCoyoteLibs SHARED pycoyote/cpp/src/cPythonLibs.cpp)
target_include_directories(pyCoyoteLibs PRIVATE ${PYBIND_INCLUDE_PATH} ${COYOTE_INCLUDE_PATH} ${PYCOYOTE_INCLUDE_PATH})

# Add Python modules
# NOTE: The module name starts with an underscore, making it a "private" module
# Then, all the Coyote Python modules are exposed as part of the pycoyote library via the main Python file, pyCoyote/__init__.py 
set(BUILD_TARGETS _cThread _cBuffer _cRcnfg)
pybind11_add_module(_cThread pycoyote/cpp/src/cThreadWrapper.cpp)
pybind11_add_module(_cBuffer pycoyote/cpp/src/cBufferWrapper.cpp)
pybind11_add_module(_cRcnfg pycoyote/cpp/src/cRcnfgWrapper.cpp)

# Include headers from Coyote and pybind11
foreach(t ${BUILD_TARGETS})
    target_include_directories(${t} PRIVATE ${PYBIND_INCLUDE_PATH} ${COYOTE_INCLUDE_PATH} ${PYCOYOTE_INCLUDE_PATH})
endforeach()

# Libraries to link against
foreach(t ${BUILD_TARGETS})
    target_link_libraries(${t} PRIVATE Coyote)
    target_link_libraries(${t} PRIVATE pyCoyoteLibs)
    target_link_directories(${t} PRIVATE /usr/local/lib)
endforeach()

# Make sure Python modules can find libCoyote.so at runtime
foreach(t ${BUILD_TARGETS})
    set_target_properties(${t} PROPERTIES
        BUILD_WITH_INSTALL_RPATH TRUE
        INSTALL_RPATH "$ORIGIN"
    )
endforeach()

# Install the bindings (_cThread, _cRcnfg) and Coyote, pyCoyoteLibs as a shared library
install(TARGETS ${BUILD_TARGETS} Coyote pyCoyoteLibs LIBRARY DESTINATION pycoyote)