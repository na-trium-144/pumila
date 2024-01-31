#include <pumila/pumila.h>
#include <pybind11/pybind11.h>
#include <pybind11/eigen.h>
#include <pybind11/stl.h>

using namespace pumila;
namespace py = pybind11;

void initPumila3Module(py::module_ &m) {
    auto pumila3 =
        py::class_<Pumila3, Pumila2, std::shared_ptr<Pumila3>>(m, "Pumila3")
            .def("make_shared", [](double l) {
                return std::make_shared<Pumila3>(l);
            });
}