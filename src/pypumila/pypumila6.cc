#include <pumila/models/pumila6.h>
#include <pybind11/pybind11.h>
#include <pybind11/eigen.h>
#include <pybind11/stl.h>

using namespace PUMILA_NS;
namespace py = pybind11;

void initPumila6Module(py::module_ &m) {
    auto pumila6 =
        py::class_<Pumila6, Pumila, std::shared_ptr<Pumila6>>(m, "Pumila6")
            .def("make_shared",
                 [](int n) { return std::make_shared<Pumila6>(n); })
            .def_readwrite("main", &Pumila6::main)
            .def_readwrite("target", &Pumila6::target)
            .def("get_action_rnd",
                 py::overload_cast<const FieldState &, double>(
                     &Pumila6::getActionRnd))
            .def("get_action_rnd",
                 py::overload_cast<const std::shared_ptr<GameSim> &, double>(
                     &Pumila6::getActionRnd))
            .def("calc_reward", &Pumila6::calcReward)
            .def("learn_step", &Pumila6::learnStep)
            .def("copy", &Pumila6::copy);
    py::class_<Pumila6::NNModel>(pumila6, "NNModel")
        .def("get_matrix_ih",
             [](const Pumila6::NNModel &model) { return *model.matrix_ih; })
        .def("get_matrix_hq",
             [](const Pumila6::NNModel &model) { return *model.matrix_hq; })
        .def("forward", &Pumila6::NNModel::forward)
        .def("backward", &Pumila6::NNModel::backward);
}