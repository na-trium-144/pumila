#include <pumila/pumila.h>
#include <pybind11/pybind11.h>
#include <pybind11/eigen.h>
#include <pybind11/stl.h>

using namespace pumila;
namespace py = pybind11;

void initPumila1Module(py::module_ &m) {
    auto pumila1 =
        py::class_<Pumila1, Pumila, std::shared_ptr<Pumila1>>(m, "Pumila1")
            .def(py::init<double, double, double>())
            .def_readwrite("main", &Pumila1::main)
            .def_readwrite("target", &Pumila1::target)
            .def("get_in_nodes", py::overload_cast<std::shared_ptr<GameSim>>(
                                     &Pumila1::getInNodes, py::const_))
            .def("calc_reward", &Pumila1::calcReward)
            .def("get_learn_action", &Pumila1::getLearnAction)
            .def("learn_result", &Pumila1::learnResult)
            .def("copy", &Pumila1::copy);
    py::class_<Pumila1::NNModel>(pumila1, "NNModel")
        .def_readwrite("matrix_ih", &Pumila1::NNModel::matrix_ih)
        .def_readwrite("matrix_hq", &Pumila1::NNModel::matrix_hq)
        .def("forward", &Pumila1::NNModel::forward)
        .def("backward", &Pumila1::NNModel::backward);
    py::class_<Pumila1::NNResult>(pumila1, "NNResult")
        .def_readwrite("in", &Pumila1::NNResult::in)
        .def_readwrite("hidden", &Pumila1::NNResult::hidden)
        .def_readwrite("q", &Pumila1::NNResult::q);

    py::class_<Pumila1N, Pumila, std::shared_ptr<Pumila1N>>(m, "Pumila1N")
        .def(py::init<int>());
}
