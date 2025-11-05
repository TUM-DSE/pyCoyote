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

#include "cThreadWrapper.hpp"

namespace pycoyote {

cThreadWrapper::cThreadWrapper(int32_t vfid, pid_t hpid, uint32_t device, pybind11::object uisr)
    : coyote::cThread(
        vfid, 
        hpid, 
        device, 
        // Note, how the uisr Python callable is wrapped into a std::function
        // that acquires the GIL before invoking the Python function in a separate thread
        // This is necessary, since the Python GIL must be held when calling Python code from C++
        // Also, we detach the thread to avoid blocking the C++ code, which can block the 
        // cThread destructor, in case Python garbage collections starts cleaning up
        //  the cThread object prematurely, leading to a deadlock.
        uisr.is_none()
            ? std::function<void(int)>(nullptr)
            : std::function<void(int)>([uisr](int val) {
                std::thread([uisr, val]() {
                    pybind11::gil_scoped_acquire gil;
                    uisr(val);
                }).detach();
            })
    ) {}

void cThreadWrapper::mapBuffer(cBuffer& buff) {
    // Coyote's userMap takes a pointer and size in bytes
    // CoyoteBuffer's size() returns number of elements
    userMap(buff.data(), buff.size() * buff.dtype().size);
}

void cThreadWrapper::unmapBuffer(cBuffer& buff) {
    userUnmap(buff.data());
}

pybind11::object cThreadWrapper::allocateBuffer(uint32_t size, std::string alloc_type, std::string dtype, uint32_t gpu_dev) {
    // Check if data type is supported
    auto it = datatype_registry.find(dtype);
    if (it == datatype_registry.end()) {
        throw std::runtime_error("Unsupported data type: " + dtype);
    }
    DataType dtype_ = it->second;

    // Allocated memory with getMem(...)
    // Coyote's getMem expects size in bytes; CoyoteBuffer's size() returns number of elements
    void* data_;
    if (alloc_type == "reg") {
        data_ = getMem({coyote::CoyoteAllocType::REG, (uint32_t) (size * dtype_.size)});
    } else if (alloc_type == "hpf") {
        data_ = getMem({coyote::CoyoteAllocType::HPF, (uint32_t) (size * dtype_.size)});
    } else if (alloc_type == "thp") {
        data_ = getMem({coyote::CoyoteAllocType::THP, (uint32_t) (size * dtype_.size)});
    } else if (alloc_type == "gpu") {
        data_ = getMem({coyote::CoyoteAllocType::GPU, (uint32_t) (size * dtype_.size), false, gpu_dev});
    } else {
        throw std::runtime_error("Unsupported memory allocation type: " + alloc_type);
    }

    // Note the slight hack when returning the CoyoteBuffer object:
    // The C++ implementation of the CoyoteBuffer (cBuffer) acts as a base for the Python CoyoteBuffer class,
    // which implements additional functionality for accessing individual elements, enumeration, prints etc.
    // If this function returned the cBuffer object directly, the additional functionality would not be 
    // available in Python. Therefore, the Python class is imported using pybind11, and an instance of that
    // class is created here, passing the allocated data pointer, size and data type.
    // However, for other functions in the cThread, which require a cBuffer instance (e.g., localRead, localWrite)
    // it's sufficiet to accept the base cBuffer class, as they only rely on low-level getter methods like data() and size().
    pybind11::object py_cls = pybind11::module_::import("pycoyote").attr("CoyoteBuffer");
    return py_cls(reinterpret_cast<uint64_t>(data_), size, dtype);
}

void cThreadWrapper::sync(cBuffer& buff, uint32_t size) {
    coyote::syncSg sg = { buff.data(), (uint32_t) (size * buff.dtype().size) };
    invoke(coyote::CoyoteOper::LOCAL_SYNC, sg);
}

void cThreadWrapper::offload(cBuffer& buff, uint32_t size) {
    coyote::syncSg sg = { buff.data(), (uint32_t) (size * buff.dtype().size) };
    invoke(coyote::CoyoteOper::LOCAL_OFFLOAD, sg);
}

void cThreadWrapper::localRead(cBuffer& buff, uint32_t size, std::string stream, uint32_t dest, bool last) {
    if (stream != "host" && stream != "card") {
        throw std::runtime_error("Invalid stream type: " + stream + ". Must be 'host' or 'card'.");
    }

    coyote::localSg sg = { 
        buff.data(), 
        (uint32_t) (size * buff.dtype().size), 
        (uint32_t) ((stream == "host") ? coyote::STRM_HOST : coyote::STRM_CARD), 
        dest 
    };
    invoke(coyote::CoyoteOper::LOCAL_READ, sg, last);
}

void cThreadWrapper::localWrite(cBuffer& buff, uint32_t size, std::string stream, uint32_t dest, bool last) {
    if (stream != "host" && stream != "card") {
        throw std::runtime_error("Invalid stream type: " + stream + ". Must be 'host' or 'card'.");
    }

    coyote::localSg sg = { 
        buff.data(), 
        (uint32_t) (size * buff.dtype().size), 
        (uint32_t) ((stream == "host") ? coyote::STRM_HOST : coyote::STRM_CARD), 
        dest 
    };
    invoke(coyote::CoyoteOper::LOCAL_WRITE, sg, last);
}

void cThreadWrapper::localTransfer(
    cBuffer& src_buff, cBuffer& dst_buff, 
    uint32_t src_size, uint32_t dst_size,
    std::string src_stream, std::string dst_stream,
    uint32_t src_dest, uint32_t dst_dest, 
    bool last
) {
    if (src_stream != "host" && src_stream != "card") {
        throw std::runtime_error("Invalid source stream type: " + src_stream + ". Must be 'host' or 'card'.");
    }
    if (dst_stream != "host" && dst_stream != "card") {
        throw std::runtime_error("Invalid destination stream type: " + dst_stream + ". Must be 'host' or 'card'.");
    }

    coyote::localSg src_sg = { 
        src_buff.data(), 
        (uint32_t) (src_size * src_buff.dtype().size), 
        (uint32_t) ((src_stream == "host") ? coyote::STRM_HOST : coyote::STRM_CARD), 
        src_dest 
    };
    coyote::localSg dst_sg = { 
        dst_buff.data(), 
        (uint32_t) (dst_size * dst_buff.dtype().size), 
        (uint32_t) ((src_stream == "host") ? coyote::STRM_HOST : coyote::STRM_CARD), 
        dst_dest 
    };

    invoke(coyote::CoyoteOper::LOCAL_TRANSFER, src_sg, dst_sg, last);
}

void cThreadWrapper::rdmaWrite(
    uint32_t size, std::string local_stream, 
    uint32_t local_dest, uint32_t local_offs, 
    uint32_t remote_dest, uint32_t remote_offs, 
    bool last
) {
    if (local_stream != "host" && local_stream != "card") {
        throw std::runtime_error("Invalid local stream type: " + local_stream + ". Must be 'host' or 'card'.");
    }

    coyote::rdmaSg sg = {
        (uint64_t) local_offs,
        (uint32_t) ((local_stream == "host") ? coyote::STRM_HOST : coyote::STRM_CARD),
        local_dest,
        (uint64_t) remote_offs,
        remote_dest,
        (uint32_t) (size * this->qp_dtype.size)
    };

    invoke(coyote::CoyoteOper::REMOTE_RDMA_WRITE, sg, last);
}

void cThreadWrapper::rdmaRead(
    uint32_t size, std::string local_stream, 
    uint32_t local_dest, uint32_t local_offs, 
    uint32_t remote_dest, uint32_t remote_offs, 
    bool last
) {
    if (local_stream != "host" && local_stream != "card") {
        throw std::runtime_error("Invalid local stream type: " + local_stream + ". Must be 'host' or 'card'.");
    }

    coyote::rdmaSg sg = {
        (uint64_t) local_offs,
        (uint32_t) ((local_stream == "host") ? coyote::STRM_HOST : coyote::STRM_CARD),
        local_dest,
        (uint64_t) remote_offs,
        remote_dest,
        (uint32_t) (size * this->qp_dtype.size)
    };

    invoke(coyote::CoyoteOper::REMOTE_RDMA_READ, sg, last);
}


uint32_t cThreadWrapper::getCompleted(std::string oper) const {
    // Convert oper to lowercase for case-insensitive comparisons
    std::transform(oper.begin(), oper.end(), oper.begin(), ::tolower);

    // Check what operation type is requested
    if (oper == "local_read") {
        return checkCompleted(coyote::CoyoteOper::LOCAL_READ);
    } else if (oper == "local_write") {
        return checkCompleted(coyote::CoyoteOper::LOCAL_WRITE);
    } else if (oper == "local_transfer") {
        return checkCompleted(coyote::CoyoteOper::LOCAL_TRANSFER);
    } else if (oper == "rdma_read") {
        return checkCompleted(coyote::CoyoteOper::REMOTE_RDMA_READ);
    } else if (oper == "rdma_write") {
        return checkCompleted(coyote::CoyoteOper::REMOTE_RDMA_WRITE);
    } else {
        std::cerr << "WARNING: Unsupported operation type for getCompleted(): " << oper << std::endl;
        return 0;
    }
}

pybind11::object cThreadWrapper::initRDMAWrapper(uint32_t buffer_size, std::string dtype, uint16_t port, pybind11::object server_address) {
    // Check if data type is supported
    auto it = pycoyote::datatype_registry.find(dtype);
    if (it == pycoyote::datatype_registry.end()) {
        throw std::runtime_error("Unsupported data type: " + dtype);
    }
    pycoyote::DataType dtype_ = it->second;

    // Store the data type, so that its size can be queried during RDMA operations
    this->qp_dtype = dtype_;

    // Map the server_address Python object to a const char*
    const char* addr = server_address.is_none() ? nullptr : server_address.cast<std::string>().c_str();

    // Set-up RDMA
    void* mem_ = initRDMA((uint32_t) (buffer_size * dtype_.size), port, addr);
    
    // Return a CoyoteBuffer object wrapping the allocated memory
    pybind11::object py_cls = pybind11::module_::import("pycoyote").attr("CoyoteBuffer");
    return py_cls(reinterpret_cast<uint64_t>(mem_), buffer_size, dtype);
}

}

PYBIND11_MODULE(_cThread, m, pybind11::mod_gil_not_used()) {
    pybind11::class_<pycoyote::cThreadWrapper>(m, "CoyoteThread")
        
        // -- constructor
        .def(
            pybind11::init<int32_t, pid_t, uint32_t, pybind11::object>(), 
            pybind11::arg("vfid"), pybind11::arg("hpid"), pybind11::arg("device") = 0,  pybind11::arg("uisr") = pybind11::none(),
            ""
        )

        // -- mapBuffer
        .def(
            "map_buffer", 
            &pycoyote::cThreadWrapper::mapBuffer, 
            pybind11::arg("buff"),
            ""
        )

        // -- unmapBuffer
        .def(
            "unmap_buffer", 
            &pycoyote::cThreadWrapper::unmapBuffer, 
            pybind11::arg("buff"),
            ""
        )

        // -- allocateBuffer
        .def(
            "allocate_buffer", 
            &pycoyote::cThreadWrapper::allocateBuffer, 
            pybind11::arg("size"), pybind11::arg("alloc_type") = "reg", pybind11::arg("dtype") = "float32", pybind11::arg("gpu_dev") = 0,
            ""
        )

        // -- setCSR
        .def(
            "set_csr", 
            &coyote::cThread::setCSR, 
            pybind11::arg("value"), pybind11::arg("offset"),
            ""
        )

        // -- getCSR
        .def(
            "get_csr", 
            &coyote::cThread::getCSR, 
            pybind11::arg("offset"),
            ""
        )

        // -- sync
        .def(
            "sync", 
            &pycoyote::cThreadWrapper::sync, 
            pybind11::arg("buff"), pybind11::arg("size"),
            ""
        )   

        // -- offload 
        .def(
            "offload", 
            &pycoyote::cThreadWrapper::offload, 
            pybind11::arg("buff"), pybind11::arg("size"),
            ""
        )

        // -- localRead
        .def(
            "local_read", 
            &pycoyote::cThreadWrapper::localRead, 
            pybind11::arg("buff"), pybind11::arg("size"), pybind11::arg("stream") = "host", pybind11::arg("dest") = 0, pybind11::arg("last") = true,
            ""
        )

        // -- localWrite
        .def(
            "local_write", 
            &pycoyote::cThreadWrapper::localWrite, 
            pybind11::arg("buff"), pybind11::arg("size"), pybind11::arg("stream") = "host", pybind11::arg("dest") = 0, pybind11::arg("last") = true,
            ""
        )

        // -- localTransfer
        .def(
            "local_transfer", 
            &pycoyote::cThreadWrapper::localTransfer, 
            pybind11::arg("src_buff"), pybind11::arg("dst_buff"), 
            pybind11::arg("src_size"), pybind11::arg("dst_size"),
            pybind11::arg("src_stream") = "host", pybind11::arg ("dst_stream") = "host",
            pybind11::arg("src_dest") = 0, pybind11::arg("dst_dest") = 0,
            pybind11::arg("last") = true,
            ""
        )

        // -- rdmaWrite
        .def(
            "rdma_write", 
            &pycoyote::cThreadWrapper::rdmaWrite, 
            pybind11::arg("size"), pybind11::arg("local_stream") = "host", 
            pybind11::arg("local_dest") = 0, pybind11::arg("local_offs") = 0, 
            pybind11::arg("remote_dest") = 0, pybind11::arg("remote_offs") = 0, 
            pybind11::arg("last") = true,
            ""
        )

        // -- rdmaRead
        .def(
            "rdma_read", 
            &pycoyote::cThreadWrapper::rdmaRead, 
            pybind11::arg("size"), pybind11::arg("local_stream") = "host", 
            pybind11::arg("local_dest") = 0, pybind11::arg("local_offs") = 0, 
            pybind11::arg("remote_dest") = 0, pybind11::arg("remote_offs") = 0, 
            pybind11::arg("last") = true,
            ""
        )
        
        // -- getCompleted
        .def(
            "get_completed", 
            &pycoyote::cThreadWrapper::getCompleted, 
            pybind11::arg("oper"),
            ""
        )

        // -- clearCompleted
        .def(
            "clear_completed", 
            &coyote::cThread::clearCompleted,
            ""
        )

        // -- connSync
        .def(
            "conn_sync", 
            &coyote::cThread::connSync, 
            pybind11::arg("client"),
            ""
        )

        // -- initRDMA
        .def(
            "init_rdma", 
            &pycoyote::cThreadWrapper::initRDMAWrapper,
            pybind11::arg("buffer_size"), pybind11::arg("dtype"), pybind11::arg("port"), pybind11::arg("server_address") = pybind11::none(),
            ""
        )

        // -- closeConn
        .def(
            "close_conn", 
            &coyote::cThread::closeConn,
            ""
        )

        // -- lock
        .def(
            "lock", 
            &coyote::cThread::lock,
            ""
        )

        // -- unlock
        .def(
            "unlock", 
            &coyote::cThread::unlock,
            ""
        )

        // -- getVfid
        .def(
            "get_vfid", 
            &coyote::cThread::getVfid,
            ""
        )

        // -- getCtid
        .def(
            "get_ctid", 
            &coyote::cThread::getCtid,
            ""
        )

        // -- getHpid
        .def(
            "get_hpid", 
            &coyote::cThread::getHpid,
            ""
        )

        // -- printDebug
        .def(
            "print_debug", 
            &coyote::cThread::printDebug,
            ""
        );
}
