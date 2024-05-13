#include <pumila/models/pumila3.h>
#include <pybind11/pybind11.h>

using namespace PUMILA_NS;
namespace py = pybind11;

void initPumila3Module(py::module_ &m) {
    auto pumila3 =
        py::class_<Pumila3, Pumila2, std::shared_ptr<Pumila3>>(m, "Pumila3")
            .def("make_shared",
                 [](double l) { return std::make_shared<Pumila3>(l); })
            .def("make_shared", [](const std::string &name) {
                return std::make_shared<Pumila3>(name);
            });
}