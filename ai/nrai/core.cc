/*************************************************************************
  > File Name:    core.cc
  > Author:       Zhu Zhenwei
  > Mail:         losted.leo@gmail.com
  > Created Time: Thu 12 Oct 2017 02:44:32 PM CST
 ************************************************************************/

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "core.h"


namespace py = pybind11;

namespace nrai {
PYBIND11_MODULE(nrai, m) {
  m.doc() = R"pbdoc(
    nrai python interface
  )pbdoc";

  py::class_<BalanceInt>(m, "Balance")
    .def(py::init<int, int>())
    .def("set_coeff", &BalanceInt::set_coeff)
    .def("swap", &BalanceInt::swap)
    .def("born", &BalanceInt::born)
    .def("connection", &BalanceInt::connection)
    .def("grow", &BalanceInt::grow)
    .def("shrink", &BalanceInt::shrink)
    .def("x", &BalanceInt::x)
    .def("y", &BalanceInt::y);
}
} // namespace nrai
