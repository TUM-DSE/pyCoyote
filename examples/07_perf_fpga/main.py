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
import argparse

from enum import Enum
from pycoyote import CoyoteThread

# Constants
CLOCK_PERIOD_NS = 4
DEFAULT_VFPGA_ID = 0

N_LATENCY_REPS = 1
N_THROUGHPUT_REPS = 32

class BenchmarkRegisters(Enum):
    CTRL_REG = 0        # AP start, read or write
    DONE_REG = 1        # Number of completed requests
    TIMER_REG = 2       # Timer register
    VADDR_REG = 3       # Buffer virtual address
    LEN_REG = 4         # Buffer length (size in bytes)
    PID_REG = 5         # Coyote thread ID
    N_REPS_REG = 6      # Number of read/write repetitions
    N_BEATS_REG = 7     # Number of expected AXI beats

# 01 written to CTRL_REG starts a read operation and 10 written to CTRL registers starts a write
class BenchmarkOperation(Enum):
    START_RD = 0x1
    START_WR = 0x2

def run_bench(coyote_thread, size, memory, transfers, n_runs, operation):
    # Single iteration of transfers (reads or writes)
    # Note, size here is in bytes, since the hardware register in vfpga_top.svh is defined to represent 
    # transfer size in bytes. Just to keep in mind, since in pyCoyote size typically refers to the number
    # of elements in the buffer. However, in this case the two are equal (sizeof(char) = 1 B)
    def benchmark_run():
        n_beats = transfers * ((size + 64 - 1) // 64)
        coyote_thread.set_csr(memory.addr(), BenchmarkRegisters.VADDR_REG.value)
        coyote_thread.set_csr(size, BenchmarkRegisters.LEN_REG.value)
        coyote_thread.set_csr(coyote_thread.get_ctid(), BenchmarkRegisters.PID_REG.value)
        coyote_thread.set_csr(transfers, BenchmarkRegisters.N_REPS_REG.value)
        coyote_thread.set_csr(n_beats, BenchmarkRegisters.N_BEATS_REG.value)

        # Start the operation by writing to the control register
        coyote_thread.set_csr(operation.value, BenchmarkRegisters.CTRL_REG.value)

        # Wait until done register is asserted high
        while not coyote_thread.get_csr(BenchmarkRegisters.DONE_REG.value):
            pass
        
        # Read from time register and convert to ns
        return coyote_thread.get_csr(BenchmarkRegisters.TIMER_REG.value) * CLOCK_PERIOD_NS

    # Run benchmark
    avg_time = 0.0
    for _ in range(n_runs):
        avg_time += benchmark_run()
    return avg_time / n_runs

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Coyote Perf FPGA Options')
    parser.add_argument('-o', '--operation', type=int, default=0, help='Benchmark operation: READ(0) or WRITE(1)')
    parser.add_argument('-r', '--runs', type=int, default=50, help='Number of times to repeat the test')
    parser.add_argument('-x', '--min_size', type=int, default=64, help='Starting (minimum) transfer size [B]')
    parser.add_argument('-X', '--max_size', type=int, default=4 * 1024 * 1024, help='Ending (maximum) transfer size [B]')
    args = parser.parse_args()

    operation = BenchmarkOperation.START_WR if args.operation else BenchmarkOperation.START_RD
    n_runs = args.runs
    min_size = args.min_size
    max_size = args.max_size

    print('\033[31mCLI PARAMETERS:\033[0m')
    print(f'Benchmark operation: {"WRITE" if args.operation else "READ"}')
    print(f'Number of test runs: {n_runs}')
    print(f'Starting transfer size: {min_size}')
    print(f'Ending transfer size: {max_size}\n')
    print()

    # Create Coyote thread and allocate source & destination memory
    # Set dtype to char, so that each element is 1 byte (simplifies throughput calculation)
    coyote_thread = CoyoteThread(DEFAULT_VFPGA_ID, os.getpid())
    memory = coyote_thread.allocate_buffer(max_size, alloc_type='hpf', dtype='char')

    # Benchmark sweep
    print('PERF FPGA')
    curr_size = min_size
    while curr_size <= max_size:
        # Run throughput test
        throughput_time = run_bench(coyote_thread, curr_size, memory, N_THROUGHPUT_REPS, n_runs, operation)
        throughput = (N_THROUGHPUT_REPS * curr_size) / (1024 * 1024 * throughput_time * 1e-9)
        
        # Run latency test
        latency_time = run_bench(coyote_thread, curr_size, memory, N_LATENCY_REPS, n_runs, operation)

        # Update size and proceed to next iteration
        print(f'Size: {curr_size}; average throughput: {round(throughput, 3)} MB/s, average latency {round(latency_time / 1e3, 3)} us')
        curr_size *= 2
