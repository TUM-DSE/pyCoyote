######################################################################################
# This file is part of pyCoyote <https://github.com/fpgasystems/pyCoyote>
# 
# MIT Licence
# Copyright (c) 2025 Systems Group, ETH Zurich
# All rights reserved.
# 
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the 'Software'), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED 'AS IS', WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
######################################################################################

import os
import time

from pycoyote import CoyoteThread

# Constants
N_ELEMENTS = 16
DEFAULT_DEVICE = 0
DEFAULT_VFPGA_ID = 0

# Define the interrupt callback routine; in this case, a simple example
# that prints the received interrupt value.
def interrupt_callback(value: int):
    print(f'Hello from my interrupt callback! The interrupt received a value: {value}')

if __name__ == '__main__':
    # Create a Coyote thread and define the interrupt callback method
    coyote_thread = CoyoteThread(DEFAULT_VFPGA_ID, os.getpid(), DEFAULT_DEVICE, interrupt_callback)

    # Allocate and initialize data
    data = coyote_thread.allocate_buffer(N_ELEMENTS, dtype='int')
    for i in range(N_ELEMENTS):
        data[i] = i

    # Run a test that will issue an interrupt
    data[0] = 73
    print('I am now starting a data transfer which will cause an interrupt...')
    coyote_thread.local_read(data, N_ELEMENTS)

    # Poll on completion of the transfer & once complete, clear
    while not coyote_thread.get_completed('local_read'):
        continue
    coyote_thread.clear_completed()

    time.sleep(1)

    # Now, run a case which won't issue an interrupt
    data[0] = 1024
    print('I am now starting a data transfer which shouldn\'t cause an interrupt...')
    coyote_thread.local_read(data, N_ELEMENTS)
    while not coyote_thread.get_completed('local_read'):
        continue
    coyote_thread.clear_completed()
    print('And, as promised, there was no interrupt!')

