/*
 * This file is part of pyCoyote <https://github.com/fpgasystems/pyCoyote>
 *
 * MIT Licence
 * Copyright (c) 2025 Systems Group, ETH Zurich
 * All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef _PYCOYOTE_CTHREAD_WRAPPER_HPP_
#define _PYCOYOTE_CTHREAD_WRAPPER_HPP_

#include "cThread.hpp"
#include "cPythonLibs.hpp"

#include "pybind11/stl.h"
#include "pybind11/pybind11.h"
#include "pybind11/functional.h"

namespace pycoyote {

/**
 * @brief Wrapper class for coyote::cThread to be used in Python bindings.
 * 
 * The cThread is the core component of Coyote for interacting with vFPGAs.
 * It provides methods for memory management, data transfer operations, 
 * and synchronization with the vFPGA device. It also handles user interrupts and 
 * out-of-band set-up for RDMA operations. The cThtread abstracts the interaction 
 * with the char vfpga_device in the driver, providing a high-level interface
 * for Coyote operations.
 *
 * The main purpose of this wrapper class is to be more Pythonic
 * For example, to allocate memory in Coyote' C++ API, the function getMem(...) is used,
 * which returns a pointer to the allocated memory. However, working with raw pointers
 * is not very Pythonic. Similarly, the invoke function takes a C++ enum, which specifies
 * the type of data movement (e.g., LOCAL_READ, REMOTE_WRITE, etc.). In Python, it is more
 * common to use strings or, even, have separate methods for each type of data movement.
 * This wrapper class implements more Pythonic interfaces while internally calling
 * the appropriate methods from coyote::cThread. Some methods from cThread
 * (e.g., setCSR, getVfid) are directly exposed without modification.
 */
class cThreadWrapper: public coyote::cThread {

private:
	pycoyote::DataType qp_dtype;

public:

    /**
	 * @brief Default constructor for the cThread
	 *
	 * @param vfid Virtual FPGA ID
	 * @param hpid Host process ID
	 * @param device Device number, for systems with multiple vFPGAs
	 * @param uisr User interrupt (notifications) service routine, called when an interrupt from the vFPGA is received
	 */
    cThreadWrapper(int32_t vfid, pid_t hpid, uint32_t device = 0, pybind11::object uisr = pybind11::none());

	/**
	 * @brief Maps a buffer to the vFPGAs TLB
	 *
	 * @param buff Coyote buffer object
	 */
	void mapBuffer(cBuffer& buff);

	/**
	 * @brief Unmaps a buffer from the the vFPGAs TLB
	 *
	 * @param buff Coyote buffer object
	 */
	void unmapBuffer(cBuffer& buff);

	/**
	 * @brief Allocate a CoyoteBuffer object and map it into the vFPGA's TLB
	 *
	 * @param size Number of elements in the buffer
	 * @param alloc_type Allocation type: "reg" (regular), "hpf" (hugepage), "thp" (transparent hugepage), "gpu" (default "reg")
	 * @param dtype Data type of the buffer, e.g. "float32" or "char" (default "float32")
	 * @param gpu_dev GPU device ID (only relevant if alloc_type == "gpu") (default 0)
	 * @return cBuffer object representing the allocated buffer
	 */
    pybind11::object allocateBuffer(uint32_t size, std::string alloc_type = "reg", std::string dtype = "float32", uint32_t gpu_dev = 0);
	
	/**
	 * @brief Sync a buffer back to host memory
	 *
	 * @param buff Coyote buffer object to be synced
	 * @param size Number of elements in the buffer to be synced
	 */
	void sync(cBuffer& buff, uint32_t size);

	/**
	 * @brief Off-load a buffer to FPGA memory
	 *
	 * @param buff Coyote buffer object to be off-loaded
	 * @param size Number of elements in the buffer to be off-loaded
	 */
	void offload(cBuffer& buff, uint32_t size);

	/**
	 * @brief Invoke a one-sided LOCAL_READ operation
	 *
	 * @param buff Coyote buffer object to be read by the vFPGA
	 * @param size Number of elements in the buffer to be read
	 * @param stream Coyote stream: "host" or "card" (default "host")
	 * @param dest Target destination stream in the vFPGA (default 0)
	 * @param last Indicates whether this is the last operation in a sequence (default: true)
	 */
	void localRead(cBuffer& buff, uint32_t size, std::string stream = "host", uint32_t dest = 0, bool last = true);

	/**
	 * @brief Invoke a one-sided LOCAL_WRITE operation
	 *
	 * @param buff Coyote buffer object to be written to from the vFPGA
	 * @param size Number of elements written to the buffer
	 * @param stream Coyote stream: "host" or "card" (default "host")
	 * @param dest Target destination stream in the vFPGA (default 0)
	 * @param last Indicates whether this is the last operation in a sequence (default: true)
	 */
	void localWrite(cBuffer& buff, uint32_t size, std::string stream = "host", uint32_t dest = 0, bool last = true);

	/**
	 * @brief Invoke a two-sided LOCAL_TRANSFER operation
	 *
	 * @param src_buff Source buffer, to be read by the vFPGA
	 * @param dst_buff Destination buffer, to be written to from the vFPGA
	 * @param src_size Number of elements to read from the source buffer
	 * @param dst_size Number of elements to write to the destination buffer
	 * @param src_stream Source buffer stream: "host" or "card" (default "host")
  	 * @param dst_stream Destination buffer stream: "host" or "card" (default "host")
	 * @param src_dest Target AXI4 stream in the vFPGA of the source buffer (default 0)
	 * @param dst_dest Target AXI4 stream in the vFPGA of the destination buffer (default 0)
	 * @param last Indicates whether this is the last operation in a sequence (default: true)
	 */
	void localTransfer(
		cBuffer& src_buff, cBuffer& dst_buff, 
		uint32_t src_size, uint32_t dst_size,
		std::string src_stream = "host", std::string dst_stream = "host",
		uint32_t src_dest = 0, uint32_t dst_dest = 0, 
		bool last = true
	);

	/**
	 * @brief Invoke a one-sided REMOTE_RDMA_WRITE operation on the buffer defined during QP exchange
	 *
	 * @param size Number of elements to write to the remote vFPGA
	 * @param local_stream Local buffer stream: "host" or "card" (default "host")
	 * @param local_dest Target AXI4 stream in the vFPGA of the local buffer (default 0)
	 * @param local_offs Offset from the local buffer address (default 0)
	 * @param remote_dest Target AXI4 stream in the remote vFPGA (default 0)
	 * @param remote_offs Offset for the remote buffer to which the data is sent (default 0)
	 * @param last Indicates whether this is the last operation in a sequence (default: true)
	 */
	void rdmaWrite(
		uint32_t size, std::string local_stream = "host", 
		uint32_t local_dest = 0, uint32_t local_offs = 0, 
		uint32_t remote_dest = 0, uint32_t remote_offs = 0, 
		bool last = true
	);
	
	/**
	 * @brief Invoke a one-sided REMOTE_RDMA_READ operation on the buffer defined during QP exchange
	 *
	 * @param size Number of elements to write to the remote vFPGA
	 * @param local_stream Local buffer stream: "host" or "card" (default "host")
	 * @param local_dest Target AXI4 stream in the vFPGA of the local buffer (default 0)
	 * @param local_offs Offset from the local buffer address (default 0)
	 * @param remote_dest Target AXI4 stream in the remote vFPGA (default 0)
	 * @param remote_offs Offset for the remote buffer to which the data is sent (default 0)
	 * @param last Indicates whether this is the last operation in a sequence (default: true)
	 */
	void rdmaRead(
		uint32_t size, std::string local_stream = "host", 
		uint32_t local_dest = 0, uint32_t local_offs = 0, 
		uint32_t remote_dest = 0, uint32_t remote_offs = 0, 
		bool last = true
	);

	/**
	 * @brief  Returns the number of completed operations for a given Coyote operation type
	 *
	 * @param oper Operation type as a string, e.g., 'local_read' or 'rdma_write'
	 * @return Cumulative number of completed operations for the specified operation type, since the last clearCompleted() call
	 */
	uint32_t getCompleted(std::string oper) const;

	/**
	 * @brief Sets up the cThread for RDMA operations
	 *
	 * This function creates an out-of-band connection to the server,
	 * which is used to exchange the queue pair (QP) between the nodes.
	 * Additionally, it allocates a buffer for the RDMA operations
	 * and returns a pointer to the allocated buffer.
	 * 
	 * @param buffer_size Size of the buffer (number of elements) to be allocated for RDMA operations
	 * @param dtype Data type of the buffer, e.g. "float32" or "char"
	 * @param port Port number to be used for the out-of-band connection
	 * @param server_address Optional server address to connect to; if not provided, this cThread acts as the server
	 * @return CoyoteBuffer object representing the allocated buffer for RDMA operations
	 */
	pybind11::object initRDMAWrapper(uint32_t buffer_size, std::string dtype, uint16_t port, pybind11::object server_address = pybind11::none());

};

}

#endif // _PYCOYOTE_CTHREAD_WRAPPER_HPP_