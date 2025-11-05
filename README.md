# pyCoyote
Python run-time for [Coyote](https://github.com/fpgasystems/Coyote), the open-source FPGA shell.

pyCoyote aims to provide high-level Python abstractions for interaction with Coyote, namely cThread and cRcnfg functionality. It can be thought of as an alternative to Coyote's C++ software API.

# Getting started
## Prerequisites
pyCoyote is complimentary to Coyote - therefore, all Coyote's [prerequisites](https://fpgasystems.github.io/Coyote/intro/quick-start.html#prerequisites) are also prerequisites of pyCoyote. 

Additionally, pyCoyote requires Python >= 3.8 with pip installed.

## Installation
Clone the repo and all its submodules:
```bash
git clone --recurse-submodules https://github.com/fpgasystems/pyCoyote
```

Then, simply install the Python package via pip (which, ideally, should be done inside a virtual environment):
```bash
cd pyCoyote
pip install .
```

**N.B.:** Installation with pip should be done on the node which will be running Coyote. Since pyCoyote relies on a compiled C++ libary, it must be recompiled between different set-ups (just like the Coyote driver and software). 

## GPU support
To build pyCoyote with support for GPU - FPGA DMA, the following command should be used. Building with GPU support allows direct data movement (bypassing the CPU) between the GPU and FPGA. These concepts are covered in [Example 6: FPGA-GPU Peer-to-Peer Data Movement.](https://github.com/fpgasystems/Coyote/tree/master/examples/06_gpu_p2p)

```bash
PYCOYOTE_EN_GPU=1 CXX=hipcc pip install .
```

Additionally, while not explicitly required for pyCoyote, installing [Python bindings for HIP](https://rocm.docs.amd.com/projects/hip-python/en/latest/) could prove useful. These allow access to standard ROCm/HIP methods for selecting GPU devices, moving data between the GPU and CPU, creating HIP streams etc. Specifically, they are required for running Example 6. Installation guides can be found on the [official website](https://rocm.docs.amd.com/projects/hip-python/en/latest/user_guide/0_install.html).

**N.B.:** To build with GPU support, the system should run Linux >= 6.2 with ROCm >= 6.0 installed. 

## Getting-started examples
The various pyCoyote examples can be found [here](https://github.com/fpgasystems/pyCoyote/tree/master/examples). These examples cover the `CoyoteThread` and `CoyoteReconfig` classes, which are Python abstractions for the C++ classes `cThread` and `cRcnfg`, enabling various Coyote functionality such as data movemement, networking and dynamic reconfiguration.

Broadly speaking, these examples are one-to-one replacements of the main Coyote examples, acting as alternatives to the software code of the main repository. Therefore, when running the examples it should be done in conjuction with the main Coyote repository: the hardware and the driver must be sythesized/compiled from Coyote, and Python code from pyCoyote be used instead of the example C++ code. In addition, detailed guides on setting up and running the examples, as well as the various Coyote concepts, can be found in the [main Coyote repository](https://github.com/fpgasystems/Coyote/tree/master/examples).

# Good-to-know
## NumPy compatibility
In addition to the abstractions for the `CoyoteThread` and `CoyoteReconfig`, pyCoyote includes `CoyoteBuffer`, a class which abstracts memory allocated in Coyote. Implemented as a `buffer_protocol` in Python, it's compatible with many standard Python libraries. In the following, we demonstrate the creation of and interaction with a `CoyoteBuffer` as well as its compatibility with NumPy (though it should work with other popular Python libraries as well). The same `CoyoteBuffer` is used for the various Coyote operations (e.g., LOCAL_TRANSFER, REMOTE_RDMA_WRITE etc.), which are covered in the respective examples.

**N.B.:** Allocating a `CoyoteBuffer` with the `allocate_buffer` function requires a bitstream to be loaded, since, internally, this function relies on the C++ `cThread` class.

```Python
import os
import numpy as np
from pycoyote import CoyoteThread

# Create a Coyote thread for vFPGA #0
coyote_thread = CoyoteThread(0, os.getpid())

# Allocate a buffer of 16 integers, with regular (4 KiB) pages
coyote_buffer = coyote_thread.allocate_buffer(16, alloc_type='reg', dtype='int')

# Initialize buffer contents
for i in range(16):
    coyote_buffer[i] = i

# Print the buffer
print(coyote_buffer)

# The buffer can be passed directly to NumPy functions, for e.g., calculating the mean
print(f'Mean value of the Coyote buffer is {np.mean(coyote_buffer)}')

# Map the CoyoteBuffer to a NumPy array with no data copies
np_arr = np.array(coyote_buffer, copy=False)

# All standard NumPy functionality is now available, e.g., reshape and transpose
print(np_arr.reshape((4, 4)).T)

# Confirm NumPy pointer and CoyoteBuffer address are equal (i.e., no data copies occured)
assert(np_arr.ctypes.data == coyote_buffer.addr())

# A CoyoteBuffer can also be constructed from an existing NumPy array,
# by simply passing the address of the data.
# N.B.: Coyote requires buffers to be aligned 64 B for data transfers, which is enforced  
# internally when creating a buffer. However, this may not be the case for NumPy-allocated
# arrays, which can lead to stalls when transferring data.
np_arr = np.random.rand(8).astype('float32')
coyote_buffer = CoyoteBuffer(np_arr.ctypes.data, 8, 'float32')
print(coyote_buffer)
```

## Differences between Coyote and pyCoyote
pyCoyote implements most of the functionality supported in Coyote's C++ API. Using the Python API is explained in the examples. The main difference between pyCoyote and Coyote is that buffers in pyCoyote are also defined by a data type (default 'float32'). In C++, allocated memory is not linked to a data type. However, by specifying the data type, CoyoteBuffers in Python become compatible with a wide range of standard Python libraries. As a consequence, in pyCoyote, **all transfers and data allocation size are expressed in number of elements, not bytes**. For e.g., a LOCAL_WRITE with a buffer of 16 integers would correspond to a transfer of 64 B and would be implemeted in pyCoyote with:
```Python
coyote_thread.local_write(buffer, 16)
``` 

## Documentation & help

# License & acknowledgment
Like Coyote, pyCoyote is licensed under the terms in [LICENSE](https://github.com/fpgasystems/pyCoyote/blob/master/LICENSE), which corresponds to the MIT Licence.
Any contributions to pyCoyote will be accepted under the same terms of license.

pyCoyote is implemented with [pybind11](https://github.com/pybind/pybind11) (BSD 3-Clause License), which is included as a git submodule in deps/.