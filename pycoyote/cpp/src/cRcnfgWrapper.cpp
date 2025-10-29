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

#include "cRcnfgWrapper.hpp"

// Bind C++ classes, functions from cRcnfg to Python
PYBIND11_MODULE(_pycoyote, m, py::mod_gil_not_used()) {
    py::class_<coyote::cRcnfg>(m, "CoyoteReconfig")
        .def(py::init<unsigned int>(), py::arg("device") = 0, "")
        .def("reconfigure_shell", &coyote::cRcnfg::reconfigureShell, py::arg("bitstream_path"), "")
        .def("reconfigure_app", &coyote::cRcnfg::reconfigureApp, py::arg("bitstream_path"), py::arg("vfid"), "");
}
