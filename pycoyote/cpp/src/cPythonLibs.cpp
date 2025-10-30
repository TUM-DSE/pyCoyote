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

#include "cPythonLibs.hpp"

namespace pycoyote {

cBuffer::cBuffer(uint64_t addr, uint32_t size, std::string dtype) {
    auto it = datatype_registry.find(dtype);
    if (it != datatype_registry.end()) {
        data_ = reinterpret_cast<void*>(addr);
        size_ = size;
        dtype_ = it->second;
    } else {
        throw std::runtime_error("Selected data type '" + dtype + "' is not supported.");
    }
}

uint64_t cBuffer::addr() const {
    return reinterpret_cast<uint64_t>(data_);
}

uint32_t cBuffer::size() const {
    return size_;
}

DataType cBuffer::dtype() const {
    return dtype_;
}

void* cBuffer::data() const {
    return data_;
}

}