#include <pumila/models/pumila12.h>
#include <pybind11/pybind11.h>
#include <pybind11/eigen.h>
#include <pybind11/stl.h>

using namespace PUMILA_NS;
namespace py = pybind11;

void initPumila12Module(py::module_ &m) {
    auto pumila12 =
        py::class_<Pumila12, Pumila, std::shared_ptr<Pumila12>>(m, "Pumila12")
            .def("make_shared",
                 [](int n, double g) {
                     return std::make_shared<Pumila12>(n, g);
                 })
            .def("make_shared",
                 [](const std::string &name) {
                     return std::make_shared<Pumila12>(name);
                 })
            .def_readwrite("main", &Pumila12::main)
            .def_readwrite("target", &Pumila12::target)
            .def_readwrite("diff_history", &Pumila12::diff_history)
            .def("get_in_nodes",
                 [](std::shared_ptr<Pumila12> pumila, const FieldState2 &field,
                    const FieldState2 &op_field) {
                     return pumila->getInNodes(field, op_field).get();
                 })
            .def("get_action_rnd",
                 py::overload_cast<const FieldState2 &,
                                   const std::optional<FieldState2> &, double>(
                     &Pumila12::getActionRnd))
            .def("get_action_rnd",
                 py::overload_cast<const std::shared_ptr<GameSim> &, double>(
                     &Pumila12::getActionRnd))
            .def("calc_reward",
                 py::overload_cast<const FieldState2 &,
                                   const std::optional<FieldState2> &>(
                     &Pumila12::calcReward, py::const_))
            .def("learn_step", &Pumila12::learnStep)
            .def("copy", &Pumila12::copy)
            .def("forward", &Pumila12::forward)
            .def("backward", &Pumila12::backward);
    py::class_<Pumila12::NNModel, std::shared_ptr<Pumila12::NNModel>>(pumila12,
                                                                      "NNModel")
        .def("get_matrix_ih",
             [](const Pumila12::NNModel &model) { return model.matrix_ih; })
        .def("get_matrix_hq",
             [](const Pumila12::NNModel &model) { return model.matrix_hq; });
}
