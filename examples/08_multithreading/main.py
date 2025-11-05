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
DEFAULT_VFPGA_ID = 0

# Registers, corresponding to the ones in aes_axi_ctrl_parser
KEY_LOW_REG = 0
KEY_HIGH_REG = 1
IV_LOW_REG = 2
IV_HIGH_REG = 3
IV_DEST_REG = 4

# 128-bit encryption key
# Partitioned into two 64-bit values, since hardware registers are 64b (8B)
KEY_LOW = 0x6167717a7a767668
KEY_HIGH = 0x6a64727366626362

# 128-bit initialization vector (IV)
# Partitioned into two 64-bit values, since hardware registers are 64b (8B)
IV_LOW = 0x6162636465666768
IV_HIGH = 0x3132333435363738

if __name__ == '__main__':
    # Parse command-line arguments
    parser = argparse.ArgumentParser(description='Coyote Multi-threaded AES Encryption Example Options')
    parser.add_argument('-t', '--threads', type=int, default=1, help='Number of Coyote threads to use')
    parser.add_argument('-r', '--runs', type=int, default=50, help='Number of times to repeat the test')
    parser.add_argument('-s', '--source_path', type=str, default='sample_text.txt', help='Text file to be encrypted')
    args = parser.parse_args()

    n_threads = args.threads
    n_runs = args.runs
    source_path = args.source_path

    if n_threads > 4:
        raise ValueError('The vFPGA is built with 4 host streams; cannot have more threads than streams in this specific example...')

    if not os.path.exists(source_path):
        raise FileNotFoundError('Could not open source text; exiting...')

    # Open source file to be encrypted
    with open(source_path, 'rb') as source_file:
        source_data = source_file.read()
    size = len(source_data)

    # Debug print
    print('\033[31mCLI PARAMETERS:\033[0m')
    print(f'Number of Coyote threads: {n_threads}')
    print(f'Number of test runs: {n_runs}')
    print(f'Text size: {size}')
    print()

    # Create multiple Coyote threads, and, for each one of them, allocated destination memory
    coyote_threads, src_buffs, dst_buffs = [], [], []
    for i in range(n_threads):
        thread = CoyoteThread(DEFAULT_VFPGA_ID, os.getpid())
        coyote_threads.append(thread)   

        src = thread.allocate_buffer(size + 1, alloc_type='hpf', dtype='char')
        for i in range(size):
            src[i] = source_data[i]
        src_buffs.append(src)

        dst = thread.allocate_buffer(size + 1, alloc_type='hpf', dtype='char')
        dst_buffs.append(dst)

    # Set the encryption key and initialization IV
    # In this case, all threads use the same key and IV
    coyote_threads[0].set_csr(KEY_LOW, KEY_LOW_REG)
    coyote_threads[0].set_csr(KEY_HIGH, KEY_HIGH_REG)
    coyote_threads[0].set_csr(IV_LOW, IV_LOW_REG)
    coyote_threads[0].set_csr(IV_HIGH, IV_HIGH_REG)

    def prep_fn():
        for thread in coyote_threads:
            thread.clear_completed()

    def benchmark_thr():
        for i, thread in enumerate(coyote_threads):
            # Encryption for the i-th thread can be started when the IV_DEST register holds the value i
            thread.set_csr(i, IV_DEST_REG)
            thread.local_transfer(
                src_buffs[i], dst_buffs[i],  # src_buff, dst_buff
                size, size,                  # src_size, dst_size
                'host', 'host',              # src_stream, dst_stream
                i, i                         # src_dest, dst_dest
            )

        while not all(thread.get_completed('local_transfer') == 1 for thread in coyote_threads):
            pass

    print('\033[31mMULTI-THREADED AES ECB ENCRYPTION\033[0m')
    total_throughput = 0
    for _ in range(n_runs):
        prep_fn()
        start_time = time.perf_counter()
        benchmark_thr()
        end_time = time.perf_counter()

        elapsed_time = end_time - start_time
        total_throughput += ((size * n_threads) / (1024 * 1024 * elapsed_time))

    avg_throughput = total_throughput / n_runs
    print(f'Average throughput: {avg_throughput:.2f} MB/s')

    # Since all the Coyote threads operate on the same text with the same key and IV,
    # confirm that the encrypted text is the same for all the threads
    for i in range(1, n_threads):
        for s in range(size):
            assert (dst_buffs[0][s] == dst_buffs[i][s])

    with open('encrypted_text.txt', 'wb') as encrypted_file:
        encrypted_file.write(bytearray(dst_buffs[0]))

