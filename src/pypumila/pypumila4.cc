#include <pumila/models/pumila4.h>
#include <pybind11/pybind11.h>

using namespace pumila;
namespace py = pybind11;

void initPumila4Module(py::module_ &m) {
    auto pumila4 =
        py::class_<Pumila4, Pumila, std::shared_ptr<Pumila4>>(m, "Pumila4")
            .def("make_shared",
                 [](double l) { return std::make_shared<Pumila4>(l); })
            .def_readwrite("main", &Pumila4::main)
            .def_readwrite("target", &Pumila4::target)
            .def("get_action_rnd",
                 py::overload_cast<const FieldState &, double>(
                     &Pumila4::getActionRnd))
            .def("get_action_rnd",
                 py::overload_cast<const std::shared_ptr<GameSim> &, double>(
                     &Pumila4::getActionRnd))
            .def("get_in_nodes",
                 [](Pumila4 &pumila4, const FieldState &field) {
                     return pumila4.getInNodes(field).get();
                 })
            .def("calc_reward", &Pumila4::calcReward)
            .def("learn_step", &Pumila4::learnStep)
            .def("copy", &Pumila4::copy);
}
