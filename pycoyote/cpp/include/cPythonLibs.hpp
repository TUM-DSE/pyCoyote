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

/**
 * @file cPythonLibs.hpp
 * @brief Functions commmon to several pyCoyote modules
 *
 * This file contains functions and definitions that are common to many 
 * components of pyCoyote's source code. For example, the CoyoteBuffer 
 * (class _cBuffer in C++)  is used as both a stand-alone 
 * Python module (coyote_buffer.py) and as part of the CoyoteThread class 
 * (class _cThreadWrapper in C++). Therefore, it is implemented in a file
 * that is compiled as a shared library and linked to both of the the modules.
 *
 * NOTE: Shared libaries cannot contain any PYBIND11_MODULE(...) definitions,
 * as it leads to failed compilation. Therefore, the cBufferWrapper.cpp
 * includes the class definition from below and only implements the Python binding
 */

#ifndef _PYCOYOTE_CPYTHON_LIBS_HPP_
#define _PYCOYOTE_CPYTHON_LIBS_HPP_

#include <string>
#include <stdexcept>
#include <unordered_map>

namespace pycoyote {

/// Metadata about a data type
struct DataType {
    std::string name;   // String-representation of a data type (e.g. "float32" or "char")
    std::string format; // Python buffer_protocol formatted string of the data type (e.g. "f" for float)
    size_t size;        // Size of the data type, in bytes; (e.g., 4 for "float32")
};

// Predefined registry of supported data types
// Map entries are string representation, Python buffer_protocol format, size in bytes
inline const std::unordered_map<std::string, DataType> datatype_registry = {
    // Bools & chars
    {"bool",   {"bool",   "?", 1}},
    {"char",   {"char",   "b", 1}},
    
    // Short integers
    {"uint8",  {"uint8",  "B", 1}},
    {"int8",   {"int8",   "b", 1}},
    {"int16",  {"int16",  "h", 2}},
    {"uint16", {"uint16", "H", 2}},
    
    // Standard integers; assume int <=> int32
    {"int",    {"int32",  "i", 4}},
    {"uint",   {"uint32", "I", 4}},
    {"int32",  {"int32",  "i", 4}},
    {"uint32", {"uint32", "I", 4}},

    // Long integers; assume long <=> int64
    {"int64",  {"int64",  "q", 8}},
    {"uint64", {"uint64", "Q", 8}},
    {"long",   {"int64",  "q", 8}},

    // Floating-point datatypes
    {"float32", {"float32", "f", 4}},
    {"float64", {"float64", "d", 8}}
};


/**
 * @brief Abstract buffer with configurable data type.
 *
 * This class encapsulates a pointer to some memory, its size 
 * (number of elements) and the data type of the elements.
 *
 * This abstraction is used to avoid exposing raw pointers in Python.
 * Coyote's C++ getMem(...) method returns a raw pointer, which would not
 * be suitable for a high-level Python API.
 *
 * Additionally, getMem(...) does not keep track of the buffer's data type. 
 * However, Python's buffer_protocol (a standard representation of buffers in Python)
 * requires the data type information (and its size). Once a class is compatible
 * with the buffer_protocol, it becomes compatible with many Python libraries (e.g., NumPy). 
 *
 * NOTE: Changing the data type after construction is currently not supported.
 *
 * NOTE: This class is not directly exposed in the Python API. 
 * Instead, it serves as a base class for CoyoteBuffer (see python/coyote_buffer.py).
 * The reason is ease of implementation. Some functionality 
 * (e.g., buffer protocol support) must be implemented in C++ using pybind11, 
 * while other functionality (e.g., accessing elements) is easier to implement 
 * in Python. For example, to access elements in C++, the getter method
 * would need to be templated based on the data type, leading to code 
 * bloat for many data types. However, in Python, accessing elements of a 
 * buffer prootocol is straightforward through memory views.
 */
class cBuffer {
    private:
        void* data_;
        uint32_t size_;
        DataType dtype_;

    public:
        /**
         * @brief Default constructor for cBuffer
         * 
         * @param addr Address of the allocated memory
         * @param size Number of elements in the buffer
         * @param dtype String representation of the buffer's data type (e.g., "float32")
         *
         * NOTE: Total size of the buffer in bytes is size * sizeof(dtype)
         * NOTE: No memory allocation or data copy is performed in this constructor.
         * Instead, this class wraps around existing memory (from for e.g. coyote::getMem(...)
         */
        cBuffer(uint64_t addr, uint32_t size, std::string dtype);

        /// @brief Returns the address of the buffer
        uint64_t addr() const;

        /// @brief Returns the size (number of elements) of the buffer
        uint32_t size() const;

        /// @brief Returns the data type of the buffer
        DataType dtype() const;

        /**
         * @brief Returns a pointer to the buffer's data
         *
         * @return Pointer to the buffer's data
         * 
         * NOTE: Not to be exposed in Python; used internally for Coyote's invoke method.
         */
        void* data() const;
};

}

#endif // _PYCOYOTE_CPYTHON_LIBS_HPP_