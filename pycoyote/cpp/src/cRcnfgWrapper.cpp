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

PYBIND11_MODULE(_cRcnfg, m, pybind11::mod_gil_not_used()) {
    pybind11::class_<coyote::cRcnfg>(m, "CoyoteReconfig")
        
        // -- constructor
        .def(
            pybind11::init<unsigned int>(), 
            pybind11::arg("device") = 0,
            ""
        )

        // -- reconfigureShell
        .def(
            "reconfigure_shell", 
            &coyote::cRcnfg::reconfigureShell, 
            pybind11::arg("bitstream_path"), 
            ""
        )
        
        // -- reconfigureApp
        .def(
            "reconfigure_app", 
            &coyote::cRcnfg::reconfigureApp, 
            pybind11::arg("bitstream_path"), pybind11::arg("vfid"), 
            ""
        );
}
