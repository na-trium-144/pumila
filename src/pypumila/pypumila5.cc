#include <pumila/models/pumila5.h>
#include <pybind11/pybind11.h>

using namespace PUMILA_NS;
namespace py = pybind11;

void initPumila5Module(py::module_ &m) {
    auto pumila5 =
        py::class_<Pumila5, Pumila3, std::shared_ptr<Pumila5>>(m, "Pumila5")
            .def("make_shared",
                 [](double l) { return std::make_shared<Pumila5>(l); })
            .def("make_shared", [](const std::string &name) {
                return std::make_shared<Pumila5>(name);
            })
            .def("calc_reward", &Pumila5::calcRewardS);
}
