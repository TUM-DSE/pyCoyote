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

from .._cBuffer import _CoyoteBufferBase

class CoyoteBuffer(_CoyoteBufferBase):
    '''
    A class representing buffers in Coyote.

    The buffer can be treated as an array-like object; elements can be accessed
    through the [] operator. Additionally, the following methods are supported:
        addr():
            Returns the address of the buffer
        size():
            Number of elements in the buffer

    CoyoteBuffers are fully compatible with NumPy - a NumPy
    array wit data from the buffer can be created with:
        arr = np.array(coyote_buffer, copy=False)

    This class extends _CoyoteBufferBase; implemented in the C++ class cBuffer
    with additional Python functions (currently, setting and getting elements).
    In the future, more functionality can be added, either in C++ or Python.
    More details and documentation can be found in cPythonLibs.hpp
    '''
    def __init__(self, *args, **kwargs):
        '''
        Default constructor for cBuffer
        
        Args:
            addr (uint64_t) Address of the allocated memory
            size (uint64_t) Number of elements in the buffer
            dtype (string, default = 'float32') String representation of the buffer's data type

        NOTE: No memory allocation or data copy is performed in the constructor.
        Instead, this class wraps around existing memory (from for e.g. coyote::getMem(...)

        NOTE: No run-time checks for verifying the address is correct and accessible; if a buffer
        is created with an invalid address, a segmentation fault or null-pointer error may occur
        '''
        super().__init__(*args, **kwargs)
        self._mv = memoryview(self)

    def __getitem__(self, i):
        return self._mv[i]

    def __setitem__(self, i, value):
        self._mv[i] = value

    def __len__(self):
        return self.size()
    
    def __iter__(self):
        for i in range(self.size()):
            yield self._mv[i]

    def __repr__(self):
        data = [self._mv[i] for i in range(self.size())]    
        return f'<CoyoteBuffer: addr={hex(self.addr())}, size={self.size()}, dtype={self.dtype()}, data={data}>'
