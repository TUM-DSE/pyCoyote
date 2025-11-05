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
import time
import random
import argparse

from pycoyote import CoyoteThread, CoyoteReconfig

# Constants
DEFAULT_DEVICE = 0
DEFAULT_VFPGA_ID = 0

###################################################
# Source code from Example 2: HLS Vector Addition #
###################################################
N_VECTOR_ELEMENTS = 1024

def run_hls_vadd():
    coyote_thread = CoyoteThread(DEFAULT_VFPGA_ID, os.getpid())
    a = coyote_thread.allocate_buffer(N_VECTOR_ELEMENTS)
    b = coyote_thread.allocate_buffer(N_VECTOR_ELEMENTS)
    c = coyote_thread.allocate_buffer(N_VECTOR_ELEMENTS)

    for i in range(N_VECTOR_ELEMENTS):
        a[i] = random.random()
        b[i] = random.random()
        c[i] = 0

    coyote_thread.local_read(a, N_VECTOR_ELEMENTS, dest = 0)
    coyote_thread.local_read(b, N_VECTOR_ELEMENTS, dest = 1)
    coyote_thread.local_write(c, N_VECTOR_ELEMENTS, dest = 0)
    while (coyote_thread.get_completed('local_read') != 2 or coyote_thread.get_completed('local_write') != 1):
        continue

    for i in range(N_VECTOR_ELEMENTS):
        assert(abs(a[i] + b[i] - c[i]) < 1e-6)
    print('Vector addition complete and correct!')

###############################################
# Source code from Example 4: User Interrupts #
###############################################
# A simple transfer for the interrupt, 16 int ~ 64 B ~ 512 b (one full AXI beat)
N_IRQ_TRANSFER_ELEMENTS = 16

def interrupt_callback(value: int):
    print(f'Hello from my interrupt callback! The interrupt received a value: {value}')

def run_user_interrupts():
    coyote_thread = CoyoteThread(DEFAULT_VFPGA_ID, os.getpid(), DEFAULT_DEVICE, interrupt_callback)

    data = coyote_thread.allocate_buffer(N_IRQ_TRANSFER_ELEMENTS, dtype='int')
    for i in range(N_IRQ_TRANSFER_ELEMENTS):
        data[i] = i

    data[0] = 73
    print('I am now starting a data transfer which will cause an interrupt...')
    coyote_thread.local_read(data, N_IRQ_TRANSFER_ELEMENTS)

    while not coyote_thread.get_completed('local_read'):
        continue
    coyote_thread.clear_completed()
    
    #  Short delay, making sure the interrupt message is printed in STDIO
    time.sleep(1)

if __name__ == '__main__':
    # Parse command-line arguments
    parser = argparse.ArgumentParser(description='Coyote Reconfigure Shell Options')
    parser.add_argument('--bitstream', '-b', type=str, required=True, help='Path to HLS Vector Add shell bitstream (.bin)')
    args = parser.parse_args()
    bitstream_path = args.bitstream

    # First, execute a kernel from the previous example, user_interrupts
    run_user_interrupts()

    # Now, reconfigure the entire shell with the one from example 2, hls_vadd 
    print(f'Reconfiguring the shell with bitstream: {bitstream_path}')
    rcnfg = CoyoteReconfig(DEFAULT_DEVICE)    
    start_time = time.time()
    rcnfg.reconfigure_shell(bitstream_path)
    end_time = time.time()
    print(f'Shell loaded in {(end_time - start_time) * 1000.0} ms')
    
    # Confirm that the shell was indeed reconfigured, by running a kernel from that shell 
    run_hls_vadd()