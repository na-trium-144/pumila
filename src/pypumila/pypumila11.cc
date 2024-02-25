#include <pumila/models/pumila11.h>
#include <pybind11/pybind11.h>
#include <pybind11/eigen.h>
#include <pybind11/stl.h>

using namespace PUMILA_NS;
namespace py = pybind11;

void initPumila11Module(py::module_ &m) {
    auto pumila11 =
        py::class_<Pumila11, Pumila, std::shared_ptr<Pumila11>>(m, "Pumila11")
            .def("make_shared",
                 [](int n, double g) {
                     return std::make_shared<Pumila11>(n, g);
                 })
            .def_readwrite("main", &Pumila11::main)
            .def_readwrite("target", &Pumila11::target)
            .def_readwrite("diff_history", &Pumila11::diff_history)
            .def(
                "get_in_nodes",
                [](std::shared_ptr<Pumila11> pumila, const FieldState2 &field) {
                    return pumila->getInNodes(field).get();
                })
            .def("get_action_rnd",
                 py::overload_cast<std::shared_ptr<FieldState2>, double>(
                     &Pumila11::getActionRnd))
            .def("get_action_rnd",
                 py::overload_cast<const std::shared_ptr<GameSim> &, double>(
                     &Pumila11::getActionRnd))
            .def("calc_reward", py::overload_cast<const FieldState2 &>(
                                    &Pumila11::calcReward, py::const_))
            .def("learn_step", &Pumila11::learnStep)
            .def("copy", &Pumila11::copy)
            .def("forward", &Pumila11::forward)
            .def("backward", &Pumila11::backward);
    py::class_<Pumila11::InFeatures>(pumila11, "InFeatures")
        .def("field", [](const Pumila11::InFeatures &feat,
                         int a) { return feat.each[a].get().field_next; })
        .def_readwrite("in_nodes", &Pumila11::InFeatures::in);
    py::class_<Pumila11::NNModel, std::shared_ptr<Pumila11::NNModel>>(pumila11,
                                                                      "NNModel")
        .def("get_matrix_ih",
             [](const Pumila11::NNModel &model) { return model.matrix_ih; })
        .def("get_matrix_hq",
             [](const Pumila11::NNModel &model) { return model.matrix_hq; });
}
