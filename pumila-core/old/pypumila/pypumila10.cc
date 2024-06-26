#include <pumila/models/pumila10.h>
#include <pybind11/pybind11.h>
#include <pybind11/eigen.h>
#include <pybind11/stl.h>

using namespace PUMILA_NS;
namespace py = pybind11;

void initPumila10Module(py::module_ &m) {
    auto pumila10 =
        py::class_<Pumila10, Pumila, std::shared_ptr<Pumila10>>(m, "Pumila10")
            .def("make_shared",
                 [](int n, double g) {
                     return std::make_shared<Pumila10>(n, g);
                 })
            .def("make_shared",
                 [](const std::string &name) {
                     return std::make_shared<Pumila10>(name);
                 })
            .def_readwrite("main", &Pumila10::main)
            .def_readwrite("target", &Pumila10::target)
            .def_readwrite("diff_history", &Pumila10::diff_history)
            .def(
                "get_in_nodes",
                [](std::shared_ptr<Pumila10> pumila, const FieldState2 &field) {
                    return pumila->getInNodes(field).get();
                })
            .def("get_action_rnd",
                 py::overload_cast<const FieldState2 &, double>(
                     &Pumila10::getActionRnd))
            .def("get_action_rnd",
                 py::overload_cast<const std::shared_ptr<GameSim> &, double>(
                     &Pumila10::getActionRnd))
            .def("calc_reward", py::overload_cast<const FieldState2 &>(
                                    &Pumila10::calcReward, py::const_))
            .def("learn_step", &Pumila10::learnStep)
            .def("copy", &Pumila10::copy)
            .def("forward", &Pumila10::forward)
            .def("backward", &Pumila10::backward);
    py::class_<Pumila10::InFeatures>(pumila10, "InFeatures")
        .def("field", [](const Pumila10::InFeatures &feat,
                         int a) { return feat.each[a].get().field_next; })
        .def_readwrite("in_nodes", &Pumila10::InFeatures::in);
}
