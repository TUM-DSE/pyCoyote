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

#include "cBufferWrapper.hpp"

PYBIND11_MODULE(_cBuffer, m, pybind11::mod_gil_not_used()) {
    pybind11::class_<pycoyote::cBuffer>(m, "_CoyoteBufferBase", pybind11::buffer_protocol())

        // -- Buffer protocol support
        .def_buffer([](pycoyote::cBuffer &self) -> pybind11::buffer_info {
            return pybind11::buffer_info(
                self.data(),                  // Pointer to buffer's data
                self.dtype().size,            // Size of one element in the buffer
                self.dtype().format,          // String representation of the data type suitable for Python's buffer_protocol
                1,                            // Number of dimensions
                { self.size() },              // Size of each dimension
                { self.dtype().size }         // Strides for each dimension
            );
        })
        
        // -- constructor
        .def(
            pybind11::init<uint64_t, uint32_t, std::string>(), 
            pybind11::arg("addr"), pybind11::arg("size"), pybind11::arg("dtype") = "float32",
            ""
        )

        // -- addr
        .def(
            "addr", 
            &pycoyote::cBuffer::addr, 
            ""
        )

        // -- size
        .def(
            "size", 
            &pycoyote::cBuffer::size, 
            ""
        )

        // -- dtype (as a string, since DataType is not exposed to Python)
        .def(
            "dtype", 
            [](const pycoyote::cBuffer &self) {
                return self.dtype().name;
            },
            ""
        );
}
