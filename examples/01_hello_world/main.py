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
import random
import argparse

from pycoyote import CoyoteThread

# Constants
INT_SIZE_BYTES = 4

DEFAULT_VFPGA_ID = 0

N_LATENCY_REPS = 1
N_THROUGHPUT_REPS = 32

# Similar implementation to example_01_hello_world/sw/main.cpp, but without warm-up runs
# Note, the lack of warm-up runs can lead to slightly different performance with HBM,
# due to the initial page fault. Additionally, in pyCoyote size typically refers to the number
# of elements in the buffer, rather than the transfer size. In this case, the size of the 
# transfer (in bytes) is 4 * size.
def run_benchmark(coyote_thread, src_buff, dst_buff, size, num_transfers, num_runs, stream):
    total_time = 0.0

    # Randomly set the source data between -512 and +512; initialise destination memory to 0
    for i in range(size):
        src_buff[i] = random.randint(-512, 512)
        dst_buff[i] = 0

    for _ in range(num_runs):
        # Clear completion counters before each run & start the timer
        coyote_thread.clear_completed()
        start_time = time.perf_counter()

        # Launch (queue) multiple, asynchronous transfers in parallel
        # The available options for src_stream and dst_stream are 'host' or 'card'
        stream_str = 'host' if stream else 'card'
        for _ in range(num_transfers):
            coyote_thread.local_transfer(
                src_buff, dst_buff, size, size, stream_str, stream_str
            )

        # Wait until all of the transfers are complete
        while coyote_thread.get_completed('local_transfer') != num_transfers:
            continue

        # Capture duration, in nanoseconds
        end_time = time.perf_counter()
        elapsed_time = (end_time - start_time) * 1e9 
        total_time += elapsed_time

    # Sync data back, if required (i.e. stream == STRM_CARD)
    if stream == 0:
        coyote_thread.sync(src_buff, size)
        coyote_thread.sync(dst_buff, size)

    # Per the vFPGA logic, make sure dst_buff = src_buff + 1
    for i in range(size):
        assert((src_buff[i] + 1) == dst_buff[i])
    
    avg_time = total_time / num_runs 
    return avg_time

if __name__ == '__main__':
    # Parse command-line arguments
    # Note, the difference to the C++ code in parameters min_size and max_size,
    # which now refer to the number of elements (integers) to transfer, rather than size in bytes.
    parser = argparse.ArgumentParser(description='Coyote Hello World Example')
    parser.add_argument('--hugepages', '-hp', action='store_true', help='Use hugepages')
    parser.add_argument('--stream', '-s', type=int, default=1, help='Data stream: HOST(1) or FPGA(0)')
    parser.add_argument('--runs', '-r', type=int, default=50, help='Number of times to repeat the test')
    parser.add_argument('--min_size', '-x', type=int, default=16, help='Starting transfer size (number of buffer elements)')
    parser.add_argument('--max_size', '-X', type=int, default=1024 * 1024, help='Ending transfer size (number of buffer elements)')
    args = parser.parse_args()

    print('\033[31mCLI PARAMETERS:\033[0m')
    print(f'Enable hugepages: {args.hugepages}')
    print(f'Data stream: {"HOST" if args.stream else "CARD"}')
    print(f'Number of test runs: {args.runs}')
    print(f'Starting transfer size: {args.min_size} integers')
    print(f'Ending transfer size: {args.max_size} integers')
    print()
    
    # Obtain a Coyote thread
    coyote_thread = CoyoteThread(DEFAULT_VFPGA_ID, os.getpid())

    # Allocate memory; note, how pyCoyote allows us to specify the buffer data type (int in this case)
    alloc_type = 'hpf' if args.hugepages else 'reg'
    src_buff = coyote_thread.allocate_buffer(args.max_size, alloc_type=alloc_type, dtype='int')
    dst_buff = coyote_thread.allocate_buffer(args.max_size, alloc_type=alloc_type, dtype='int')

    curr_size = args.min_size
    while curr_size <= args.max_size:        
        # Run throughput test
        throughput_time = run_benchmark(
            coyote_thread, src_buff, dst_buff, curr_size, N_THROUGHPUT_REPS, args.runs, args.stream
        )
        throughput = (N_THROUGHPUT_REPS * curr_size * INT_SIZE_BYTES) / (throughput_time * 1e-9 * 1024 * 1024) # MB/s

        # Run latency test
        latency = run_benchmark(
            coyote_thread, src_buff, dst_buff, curr_size, N_LATENCY_REPS, args.runs, args.stream
        )

        # Update size and proceed to next iteration
        print(f'Size: {curr_size}; average throughput: {round(throughput, 3)} MB/s, average latency {round(latency / 1e3, 3)} us')
        curr_size *= 2
