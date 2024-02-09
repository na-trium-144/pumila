#include <pumila/models/pumila7.h>
#include <pybind11/pybind11.h>

using namespace PUMILA_NS;
namespace py = pybind11;

void initPumila7Module(py::module_ &m) {
    auto pumila7 =
        py::class_<Pumila7, Pumila6, std::shared_ptr<Pumila7>>(m, "Pumila7")
            .def("make_shared",
                 [](double l) { return std::make_shared<Pumila7>(l); })
            .def("calc_reward", &Pumila7::calcReward)
            .def("calc_reward", &Pumila7::calcRewardS);
}
