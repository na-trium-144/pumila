#include <pumila/pumila.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

using namespace PUMILA_NS;
namespace py = pybind11;

PYBIND11_MODULE(pypumila_core, m) {
    py::enum_<Puyo>(m, "Puyo")
        .value("none", Puyo::none)
        .value("red", Puyo::red)
        .value("blue", Puyo::blue)
        .value("green", Puyo::green)
        .value("yellow", Puyo::yellow)
        .value("purple", Puyo::purple)
        .value("garbage", Puyo::garbage)
        .export_values();
    auto action = py::class_<Action>(m, "Action")
                      .def(py::init<>())
                      .def(py::init<int, Action::Rotation>())
                      .def_readwrite("x", &Action::x)
                      .def_readwrite("rot", &Action::rot)
                      .def("__repr__", [](const Action &a) {
                          return "<Action x = " + std::to_string(a.x) +
                                 ", rot = " +
                                 std::to_string(static_cast<int>(a.rot)) + ">";
                      });
    py::enum_<Action::Rotation>(action, "Rotation")
        .value("vertical", Action::Rotation::vertical)
        .value("vertical_inverse", Action::Rotation::vertical_inverse)
        .value("horizontal_right", Action::Rotation::horizontal_right)
        .value("horizontal_left", Action::Rotation::horizontal_left)
        .export_values();
    m.attr("actions") = actions;
    py::class_<PuyoPair, Action>(m, "PuyoPair")
        .def(py::init<>())
        .def(py::init<Puyo, Puyo>())
        .def(py::init<Puyo, Puyo, Action>())
        .def(py::init<PuyoPair, Action>())
        .def_readwrite("bottom", &PuyoPair::bottom)
        .def_readwrite("top", &PuyoPair::top)
        .def_readwrite("x", &PuyoPair::x)
        .def_readwrite("y", &PuyoPair::y)
        .def_readwrite("rot", &PuyoPair::rot)
        .def("bottom_x", &PuyoPair::bottomX)
        .def("bottom_y", &PuyoPair::bottomY)
        .def("top_x", &PuyoPair::topX)
        .def("top_y", &PuyoPair::topY)
        .def("rotate", &PuyoPair::rotate)
        .def("__repr__", [](const PuyoPair &a) {
            return "<PuyoPair bottom = " +
                   std::to_string(static_cast<int>(a.bottom)) +
                   ", top = " + std::to_string(static_cast<int>(a.top)) +
                   ", x = " + std::to_string(a.x) +
                   ", y = " + std::to_string(a.y) +
                   ", rot = " + std::to_string(static_cast<int>(a.rot)) + ">";
        });
    py::class_<FieldState3, std::shared_ptr<FieldState3>>(m, "FieldState3")
        .def(py::init<>());
    py::class_<Chain>(m, "Chain")
        .def_readwrite("connections", &Chain::connections)
        .def_readwrite("chain_num", &Chain::chain_num)
        .def("is_empty", &Chain::isEmpty)
        .def("connection_num", &Chain::connectionNum)
        .def("score", &Chain::score);
    py::class_<GameSim, std::shared_ptr<GameSim>>(m, "GameSim")
        .def(py::init<>())
        .def_readonly("field_copy", &GameSim::field)
        .def_readwrite("enable_garbage", &GameSim::enable_garbage)
        .def_readwrite("is_over", &GameSim::is_over)
        .def("set_opponent_sim", &GameSim::setOpponentSim)
        .def("move_pair", &GameSim::movePair)
        .def("rot_pair", &GameSim::rotPair)
        .def("quick_drop", &GameSim::quickDrop)
        .def("soft_drop", &GameSim::softDrop)
        .def("step", &GameSim::step)
        .def("put", &GameSim::put)
        .def("soft_put", &GameSim::softPut)
        .def("is_free_phase", &GameSim::isFreePhase);
}
