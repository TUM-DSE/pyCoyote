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

import os
import random
import argparse

from pycoyote import CoyoteThread

DEFAULT_VFPGA_ID = 0

if __name__ == '__main__':
    # Parse command-line arguments
    parser = argparse.ArgumentParser(description='Coyote HLS Vector Add Example')
    parser.add_argument('--size', '-s', type=int, default=1024, help='Vector size')
    args = parser.parse_args()
    size = args.size
    print(f'Starting HLS vector addition with {size} elements...')

    # Create a Coyote thread and allocate memory for it
    coyote_thread = CoyoteThread(DEFAULT_VFPGA_ID, os.getpid())
    a = coyote_thread.allocate_buffer(size)
    b = coyote_thread.allocate_buffer(size)
    c = coyote_thread.allocate_buffer(size)

    # Initialize input vectors with random values and output to zero
    for i in range(size):
        a[i] = random.random()
        b[i] = random.random()
        c[i] = 0

    # Run kernel and wait for completion
    coyote_thread.clear_completed()
    coyote_thread.local_read(a, size, dest = 0)
    coyote_thread.local_read(b, size, dest = 1)
    coyote_thread.local_write(c, size, dest = 0)
    while (coyote_thread.get_completed('local_read') != 2 or coyote_thread.get_completed('local_write') != 1):
        continue

    # Verify correctness of the results
    for i in range(size):
        assert(abs(a[i] + b[i] - c[i]) < 1e-6)
    print('Vector addition complete and correct!')
