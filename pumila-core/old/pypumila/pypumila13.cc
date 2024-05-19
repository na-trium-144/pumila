#include <pumila/models/pumila13.h>
#include <pybind11/pybind11.h>
#include <pybind11/eigen.h>
#include <pybind11/stl.h>

using namespace PUMILA_NS;
namespace py = pybind11;

void initPumila13Module(py::module_ &m) {
    auto pumila13 =
        py::class_<Pumila13, Pumila, std::shared_ptr<Pumila13>>(m, "Pumila13")
            .def("make_shared",
                 [](int n, double g) {
                     return std::make_shared<Pumila13>(n, g);
                 })
            .def("make_shared",
                 [](const std::string &name) {
                     return std::make_shared<Pumila13>(name);
                 })
            .def_readwrite("main", &Pumila13::main)
            .def_readwrite("target", &Pumila13::target)
            .def_readwrite("diff_history", &Pumila13::diff_history)
            .def("get_in_nodes",
                 [](std::shared_ptr<Pumila13> pumila, const FieldState2 &field,
                    const FieldState2 &op_field) {
                     return pumila->getInNodes(field, op_field).get();
                 })
            .def("get_action_rnd",
                 py::overload_cast<const FieldState2 &,
                                   const std::optional<FieldState2> &, double>(
                     &Pumila13::getActionRnd))
            .def("get_action_rnd",
                 py::overload_cast<const std::shared_ptr<GameSim> &, double>(
                     &Pumila13::getActionRnd))
            .def("calc_reward",
                 py::overload_cast<const FieldState2 &,
                                   const std::optional<FieldState2> &>(
                     &Pumila13::calcReward, py::const_))
            .def("learn_step", &Pumila13::learnStep)
            .def("copy", &Pumila13::copy)
            .def("forward", &Pumila13::forward)
            .def("backward", &Pumila13::backward);
    py::class_<Pumila13::NNModel, std::shared_ptr<Pumila13::NNModel>>(pumila13,
                                                                      "NNModel")
        .def("get_matrix_ih",
             [](const Pumila13::NNModel &model) { return model.matrix_ih; })
        .def("get_matrix_hq",
             [](const Pumila13::NNModel &model) { return model.matrix_hq; });
}
