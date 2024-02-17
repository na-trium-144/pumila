#include <pumila/models/pumila8r.h>
#include <pybind11/pybind11.h>

using namespace PUMILA_NS;
namespace py = pybind11;

void initPumila8rModule(py::module_ &m) {
    auto pumila8r =
        py::class_<Pumila8r, Pumila7r, std::shared_ptr<Pumila8r>>(m, "Pumila8r")
            .def("make_shared",
                 [](double l) { return std::make_shared<Pumila8r>(l); });
}
