#include <pumila/models/pumila8s.h>
#include <pybind11/pybind11.h>
#include <pybind11/eigen.h>
#include <pybind11/stl.h>

using namespace PUMILA_NS;
namespace py = pybind11;

void initPumila8sModule(py::module_ &m) {
    auto pumila8s =
        py::class_<Pumila8s, Pumila, std::shared_ptr<Pumila8s>>(m, "Pumila8s")
            .def("make_shared",
                 [](int n) { return std::make_shared<Pumila8s>(n); })
            .def("make_shared",
                 [](const std::string &name) {
                     return std::make_shared<Pumila8s>(name);
                 })
            .def_readwrite("main", &Pumila8s::main)
            .def_readwrite("target", &Pumila8s::target)
            .def("get_action_rnd",
                 py::overload_cast<std::shared_ptr<FieldState>, double>(
                     &Pumila8s::getActionRnd))
            .def("get_action_rnd",
                 py::overload_cast<const std::shared_ptr<GameSim> &, double>(
                     &Pumila8s::getActionRnd))
            .def("calc_reward", &Pumila8s::calcRewardS)
            .def("learn_step", &Pumila8s::learnStep)
            .def("copy", &Pumila8s::copy)
            .def("forward", &Pumila8s::forward)
            .def("backward", &Pumila8s::backward);
    py::class_<Pumila8s::NNModel, std::shared_ptr<Pumila8s::NNModel>>(pumila8s,
                                                                      "NNModel")
        .def("get_matrix_ih",
             [](const Pumila8s::NNModel &model) { return model.matrix_ih; })
        .def("get_matrix_hq",
             [](const Pumila8s::NNModel &model) { return model.matrix_hq; });
}