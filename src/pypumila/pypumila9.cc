#include <pumila/models/pumila9.h>
#include <pybind11/pybind11.h>

using namespace PUMILA_NS;
namespace py = pybind11;

void initPumila9Module(py::module_ &m) {
    auto pumila9 =
        py::class_<Pumila9, Pumila8s, std::shared_ptr<Pumila9>>(m, "Pumila9")
            .def("make_shared",
                 [](double l) { return std::make_shared<Pumila9>(l); })
            .def("calc_reward", Pumila9::calcRewardS);
}
