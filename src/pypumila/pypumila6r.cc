#include <pumila/models/pumila6r.h>
#include <pybind11/pybind11.h>
#include <pybind11/eigen.h>
#include <pybind11/stl.h>

using namespace PUMILA_NS;
namespace py = pybind11;

void initPumila6rModule(py::module_ &m) {
    auto pumila6r =
        py::class_<Pumila6r, Pumila, std::shared_ptr<Pumila6r>>(m, "Pumila6r")
            .def("make_shared",
                 [](int n) { return std::make_shared<Pumila6r>(n); })
            .def_readwrite("main", &Pumila6r::main)
            .def_readwrite("target", &Pumila6r::target)
            .def("get_action_rnd",
                 py::overload_cast<std::shared_ptr<FieldState>, double>(
                     &Pumila6r::getActionRnd))
            .def("get_action_rnd",
                 py::overload_cast<const std::shared_ptr<GameSim> &, double>(
                     &Pumila6r::getActionRnd))
            .def("calc_reward", &Pumila6r::calcReward)
            .def("learn_step", &Pumila6r::learnStep)
            .def("copy", &Pumila6r::copy);
    py::class_<Pumila6r::NNModel, std::shared_ptr<Pumila6r::NNModel>>(pumila6r,
                                                                      "NNModel")
        .def("get_matrix_ih",
             [](const Pumila6r::NNModel &model) { return model.matrix_ih; })
        .def("get_matrix_hq",
             [](const Pumila6r::NNModel &model) { return model.matrix_hq; })
        .def("forward", &Pumila6r::NNModel::forward)
        .def("backward", &Pumila6r::NNModel::backward);
}