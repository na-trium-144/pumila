#include <pumila/models/pumila8.h>
#include <pybind11/pybind11.h>

using namespace PUMILA_NS;
namespace py = pybind11;

void initPumila8Module(py::module_ &m) {
    auto pumila8 =
        py::class_<Pumila8, Pumila7, std::shared_ptr<Pumila8>>(m, "Pumila8")
            .def("make_shared",
                 [](double l) { return std::make_shared<Pumila8>(l); });
}
