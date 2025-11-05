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
import argparse

from pycoyote import CoyoteThread

# Constants
INT_SIZE_BYTES = 4

DEFAULT_VFPGA_ID = 0

N_LATENCY_REPS = 1
N_THROUGHPUT_REPS = 32

def run_benchmark(coyote_thread, rdma_buff, size, num_transfers, num_runs, operation):
    # When writing, the server asserts the written payload is correct (which the client sets)
    # When reading, the client asserts the read payload is correct (which the server sets)
    for i in range(size):
        rdma_buff[i] = i if operation else 0

    # Run benchmark
    total_time = 0.0
    for _ in range(num_runs):
        # Clear completion counters & sync connection
        coyote_thread.clear_completed()
        coyote_thread.conn_sync(True)
        start_time = time.perf_counter()

        # Issue RDMA requests
        if operation:
            for _ in range(num_transfers):
                coyote_thread.rdma_write(size)
        else:
            for _ in range(num_transfers):
                coyote_thread.rdma_read(size)
        
        # Wait on completion
        while coyote_thread.get_completed('local_write') != num_transfers:
            pass

        # Capture duration, in nanoseconds
        end_time = time.perf_counter()
        elapsed_time = (end_time - start_time) * 1e9 
        total_time += elapsed_time

    # Verify correctness for READ operations
    if not operation:
        for i in range(size):
            assert (rdma_buff[i] == i)

    avg_time = total_time / num_runs 
    avg_time = avg_time / (1.0 + operation)
    return avg_time

if __name__ == '__main__':
    # Parse command-line arguments
    parser = argparse.ArgumentParser(description='Coyote Perf RDMA Options')
    parser.add_argument('-i', '--ip_address', required=True, help='Server IP address')
    parser.add_argument('-o', '--operation', type=bool, default=False, help='Benchmark operation: READ(0) or WRITE(1)')
    parser.add_argument('-r', '--runs', type=int, default=10, help='Number of times to repeat the test')
    parser.add_argument('-x', '--min_size', type=int, default=16, help='Starting transfer size (number of buffer elements)')
    parser.add_argument('-X', '--max_size', type=int, default=262144, help='Ending transfer size (number of buffer elements)')
    args = parser.parse_args()

    print('\033[31mCLI PARAMETERS:\033[0m')
    print(f'Server IP address: {args.ip_address}')
    print(f'Benchmark operation: {"WRITE" if args.operation else "READ"}')
    print(f'Number of test runs: {args.runs}')
    print(f'Starting transfer size: {args.min_size} integers')
    print(f'Ending transfer size: {args.max_size} integers\n')

    # Obtain a Coyote thread and set-up RDMA connection
    coyote_thread = CoyoteThread(DEFAULT_VFPGA_ID, os.getpid())
    rdma_buff = coyote_thread.init_rdma(args.max_size, 'int', 18488, args.ip_address)

    print('\033[31mRDMA BENCHMARK: CLIENT\033[0m')
    curr_size = args.min_size
    while curr_size <= args.max_size:
        # Run throughput test
        throughput_time = run_benchmark(
            coyote_thread, rdma_buff, curr_size, N_THROUGHPUT_REPS, args.runs, args.operation
        )
        throughput = (N_THROUGHPUT_REPS * curr_size * INT_SIZE_BYTES) / (throughput_time * 1e-9 * 1024 * 1024) # MB/s

        # Run latency test
        latency = run_benchmark(
            coyote_thread, rdma_buff, curr_size, N_LATENCY_REPS, args.runs, args.operation
        )

        # Update size and proceed to next iteration
        print(f'Size: {curr_size}; average throughput: {round(throughput, 3)} MB/s, average latency {round(latency / 1e3, 3)} us')
        curr_size *= 2

    # Final sync and exit
    coyote_thread.conn_sync(True)
