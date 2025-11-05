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

# Registers, corresponding to the ones in aes_axi_ctrl_parser
KEY_LOW_REG = 0
KEY_HIGH_REG = 1

# Multiple transfers in parallel, for throughput test
BATCHED_TRANSFERS = 256

# 128-bit encryption key
# Partitioned into two 64-bit values, since hardware registers are 64b (8B)
KEY_LOW = 0x6167717a7a767668
KEY_HIGH = 0x6a64727366626362

if __name__ == '__main__':
    # Parse command-line arguments
    parser = argparse.ArgumentParser(description='Coyote multi-tenant AES encryption options')
    parser.add_argument('-r', '--runs', type=int, default=50, help='Number of times to repeat the test')
    parser.add_argument('-s', '--message_size', type=int, default=32 * 1024, help='Message size to be encrypted')
    parser.add_argument('-n', '--n_vfpga', type=int, default=1, help='Number of Coyote vFPGAs to use simultaneously')
    args = parser.parse_args()

    n_runs = args.runs
    n_vfpga = args.n_vfpga
    message_size = args.message_size

    print('\033[31mCLI PARAMETERS:\033[0m')
    print(f'Number of Coyote vFPGAs: {n_vfpga}')
    print(f'Number of test runs: {n_runs}')
    print(f'Message size: {message_size}')

    coyote_threads, src_buffs, dst_buffs = [], [], []
    for i in range(n_vfpga):
        # Create one Coyote thread for each vFPGA
        cthread = CoyoteThread(i, os.getpid())
        coyote_threads.append(cthread)

        # Allocate memory for each Coyote thread
        src = cthread.allocate_buffer(message_size, alloc_type='hpf', dtype='char')
        dst = cthread.allocate_buffer(message_size, alloc_type='hpf', dtype='char')
        src_buffs.append(src)
        dst_buffs.append(dst)
        
        # Initialize source to random values
        for k in range(message_size):
            src[k] = random.randint(65, 90) 

        # Set the encryption keys
        cthread.set_csr(KEY_LOW, KEY_LOW_REG)
        cthread.set_csr(KEY_HIGH, KEY_HIGH_REG)

    latencies = [[] for _ in range(n_vfpga)]
    for _ in range(n_runs):
        # Clear the completion counters for the next iteration of the benchmark
        for thread in coyote_threads:
            thread.clear_completed()

        # Reset timers
        start_times = [time.perf_counter() for _ in range(n_vfpga)]

        # Start asynchronous transfer for each thread      
        for _ in range(BATCHED_TRANSFERS):
            for i, thread in enumerate(coyote_threads):
                thread.local_transfer(src_buffs[i], dst_buffs[i], message_size, message_size)

        # Wait until all the parallel regions are complete; as each finishes, timestamp
        done = [False] * n_vfpga
        while not all(done):
            for i, thread in enumerate(coyote_threads):
                if thread.get_completed('local_transfer') == BATCHED_TRANSFERS and not done[i]:
                    done[i] = True
                    end_time = time.perf_counter()
                    latencies[i].append(end_time - start_times[i])

    for i in range(n_vfpga):
        throughput = sum((message_size * BATCHED_TRANSFERS) / (latency * 1024 * 1024) for latency in latencies[i]) / len(latencies[i])
        print(f'Average throughput for vFPGA {i} is {throughput:.2f} MB/s')
