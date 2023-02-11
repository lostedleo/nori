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

#define PYBalance(TYPE)                               \
  py::class_<Balance##TYPE>(m, "Balance"#TYPE)        \
    .def(py::init<int, int>())                        \
    .def("set_coeff", &Balance##TYPE::set_coeff)      \
    .def("swap", &Balance##TYPE::swap)                \
    .def("born", &Balance##TYPE::born)                \
    .def("connection", &Balance##TYPE::connection)    \
    .def("grow", &Balance##TYPE::grow)                \
    .def("shrink", &Balance##TYPE::shrink)            \
    .def("x", &Balance##TYPE::x)                      \
    .def("y", &Balance##TYPE::y);


namespace nrai {
PYBIND11_MODULE(_nrai, m) {
  m.doc() = R"pbdoc(
    _nrai python interface
  )pbdoc";

PYBalance(Int)
PYBalance(Float)
PYBalance(Double)
}
} // namespace nrai
