#include <pumila/models/pumila2.h>
#include <pybind11/pybind11.h>
#include <pybind11/eigen.h>
#include <pybind11/stl.h>

using namespace PUMILA_NS;
namespace py = pybind11;

void initPumila2Module(py::module_ &m) {
    auto pumila2 =
        py::class_<Pumila2, Pumila, std::shared_ptr<Pumila2>>(m, "Pumila2")
            .def("make_shared",
                 [](double a, double g, double l) {
                     return std::make_shared<Pumila2>(a, g, l);
                 })
            .def_readwrite("main", &Pumila2::main)
            .def_readwrite("target", &Pumila2::target)
            .def_readwrite("mean_diff", &Pumila2::mean_diff)
            .def("get_action_rnd",
                 py::overload_cast<std::shared_ptr<FieldState>, double>(
                     &Pumila2::getActionRnd))
            .def("get_action_rnd",
                 py::overload_cast<const std::shared_ptr<GameSim> &, double>(
                     &Pumila2::getActionRnd))
            .def("get_in_nodes",
                 [](Pumila2 &pumila2, std::shared_ptr<FieldState> field) {
                     return pumila2.getInNodes(field).get();
                 })
            .def("calc_reward", &Pumila2::calcReward)
            .def("calc_reward", &Pumila2::calcRewardS)
            .def("learn_step", &Pumila2::learnStep)
            .def("copy", &Pumila2::copy);
    py::class_<Pumila2::NNModel>(pumila2, "NNModel")
        .def("get_matrix_ih",
             [](const Pumila2::NNModel &model) { return *model.matrix_ih; })
        .def("get_matrix_hq",
             [](const Pumila2::NNModel &model) { return *model.matrix_hq; })
        .def("transpose_in_nodes", &Pumila2::NNModel::transposeInNodes)
        .def("forward", &Pumila2::NNModel::forward)
        .def("backward", &Pumila2::NNModel::backward);
    py::class_<Pumila2::NNResult>(pumila2, "NNResult")
        .def_readwrite("in", &Pumila2::NNResult::in)
        .def_readwrite("hidden", &Pumila2::NNResult::hidden)
        .def_readwrite("q", &Pumila2::NNResult::q);
    py::class_<Pumila2::InFeatures>(pumila2, "InFeatures")
        .def_readwrite("field_next", &Pumila2::InFeatures::field_next)
        .def_readwrite("in_nodes", &Pumila2::InFeatures::in);
}