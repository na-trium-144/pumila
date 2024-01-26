#include <pumila/pumila.h>
#include <pybind11/pybind11.h>

using namespace pumila;
namespace py = pybind11;

PYBIND11_MODULE(pypumila, m) {
    py::enum_<Puyo>(m, "Puyo")
        .value("none", Puyo::none)
        .value("red", Puyo::red)
        .value("blue", Puyo::blue)
        .value("green", Puyo::green)
        .value("yellow", Puyo::yellow)
        .value("purple", Puyo::purple)
        .value("garbage", Puyo::garbage)
        .export_values();
    py::class_<PuyoPair>(m, "PuyoPair")
        .def_readwrite("bottom", &PuyoPair::bottom)
        .def_readwrite("top", &PuyoPair::top)
        .def_readwrite("x", &PuyoPair::x)
        .def_readwrite("y", &PuyoPair::y)
        .def_readwrite("rot", &PuyoPair::rot)
        .def("bottom_x", &PuyoPair::bottomX)
        .def("bottom_y", &PuyoPair::bottomY)
        .def("top_x", &PuyoPair::topX)
        .def("top_y", &PuyoPair::topY)
        .def("rotate", &PuyoPair::rotate);
    py::class_<FieldState>(m, "FieldState")
        .def("copy", &FieldState::copy)
        .def("get", &FieldState::get)
        .def("put", py::overload_cast<const PuyoPair &>(&FieldState::put))
        .def("delete_chain", &FieldState::deleteChain)
        .def("calc_chain_all", &FieldState::calcChainAll)
        .def("fall", &FieldState::fall);
    py::class_<Chain>(m, "Chain")
        .def_readwrite("connections", &Chain::connections)
        .def_readwrite("chain_num", &Chain::chain_num)
        .def("is_empty", &Chain::isEmpty)
        .def("connection_num", &Chain::connectionNum)
        .def("score", &Chain::score);
    py::class_<GameSim>(m, "GameSim")
        .def_readwrite("field", &GameSim::field)
        .def_readwrite("next_pair", &GameSim::next_pair)
        .def_readwrite("next2_pair", &GameSim::next2_pair)
        .def_readwrite("score", &GameSim::score)
        .def_readwrite("current_chain", &GameSim::current_chain)
        .def("move_pair", &GameSim::movePair)
        .def("rot_pair", &GameSim::rotPair)
        .def("quick_drop", &GameSim::quickDrop)
        .def("soft_drop", &GameSim::softDrop)
        .def("step", &GameSim::step);
    py::class_<Window>(m, "Window").def("loop", &Window::loop);
    py::class_<Pumila>(m, "Pumila")
        .def("forward", &Pumila::forward)
        .def("backward", &Pumila::backward);
    py::class_<Pumila1, Pumila>(m, "Pumila1").def(py::init<double, double>());
}
