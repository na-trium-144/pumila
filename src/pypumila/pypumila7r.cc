#include <pumila/models/pumila7r.h>
#include <pybind11/pybind11.h>

using namespace PUMILA_NS;
namespace py = pybind11;

void initPumila7rModule(py::module_ &m) {
    auto pumila7r =
        py::class_<Pumila7r, Pumila6r, std::shared_ptr<Pumila7r>>(m, "Pumila7r")
            .def("make_shared",
                 [](double l) { return std::make_shared<Pumila7r>(l); })
            .def("calc_reward", &Pumila7r::calcReward)
            .def("calc_reward", &Pumila7::calcRewardS);
}
