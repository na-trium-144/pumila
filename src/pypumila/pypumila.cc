#include <pumila/pumila.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

using namespace pumila;
namespace py = pybind11;

void initPumila1Module(py::module_ &m);
void initPumila2Module(py::module_ &m);
void initPumila3Module(py::module_ &m);
void initPumila4Module(py::module_ &m);
void initPumila5Module(py::module_ &m);

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
    auto action = py::class_<Action>(m, "Action")
                      .def(py::init<>())
                      .def(py::init<double, Action::Rotation>())
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
    py::class_<FieldState>(m, "FieldState")
        .def(py::init<>())
        .def_readwrite("field", &FieldState::field)
        .def_readwrite("total_score", &FieldState::total_score)
        .def_readwrite("next", &FieldState::next)
        .def_readwrite("prev_chain_num", &FieldState::prev_chain_num)
        .def_readwrite("prev_chain_score", &FieldState::prev_chain_score)
        .def_readwrite("is_valid", &FieldState::is_valid)
        .def("copy", &FieldState::copy)
        .def("get", &FieldState::get)
        .def("put", py::overload_cast<std::size_t, std::size_t, Puyo>(
                        &FieldState::put))
        .def("put", py::overload_cast<const PuyoPair &>(&FieldState::put))
        .def("put", py::overload_cast<>(&FieldState::put))
        .def("put", py::overload_cast<const PuyoPair &, const Action &>(
                        &FieldState::put))
        .def("put", py::overload_cast<const Action &>(&FieldState::put))
        .def("get_height", py::overload_cast<const PuyoPair &, bool>(
                               &FieldState::getHeight, py::const_))
        .def("get_height",
             py::overload_cast<std::size_t>(&FieldState::getHeight, py::const_))
        .def("delete_connection", py::overload_cast<std::size_t, std::size_t>(
                                      &FieldState::deleteConnection))
        .def("delete_chain", &FieldState::deleteChain)
        .def("delete_chain_recurse", &FieldState::deleteChainRecurse)
        .def("calc_chain_all", &FieldState::calcChainAll)
        .def("fall", &FieldState::fall);
    py::class_<Chain>(m, "Chain")
        .def_readwrite("connections", &Chain::connections)
        .def_readwrite("chain_num", &Chain::chain_num)
        .def("is_empty", &Chain::isEmpty)
        .def("connection_num", &Chain::connectionNum)
        .def("score", &Chain::score);
    py::class_<GameSim, std::shared_ptr<GameSim>>(m, "GameSim")
        .def(py::init<>())
        .def_readwrite("field", &GameSim::field)
        .def_readwrite("current_chain", &GameSim::current_chain)
        .def("move_pair", &GameSim::movePair)
        .def("rot_pair", &GameSim::rotPair)
        .def("quick_drop", &GameSim::quickDrop)
        .def("soft_drop", &GameSim::softDrop)
        .def("step", &GameSim::step)
        .def("put", &GameSim::put)
        .def("is_free_phase", &GameSim::isFreePhase);
    py::class_<Window>(m, "Window")
        .def(py::init<std::shared_ptr<GameSim>>())
        .def("step", &Window::step)
        .def("quit", &Window::quit)
        .def("is_running", &Window::isRunning);
    py::class_<Pumila, std::shared_ptr<Pumila>>(m, "Pumila")
        .def("get_action",
             py::overload_cast<const FieldState &>(&Pumila::getAction))
        .def("get_action", py::overload_cast<const std::shared_ptr<GameSim> &>(
                               &Pumila::getAction))
        .def("load_file", &Pumila::loadFile)
        .def("save_file", &Pumila::saveFile);

    initPumila1Module(m);
    initPumila2Module(m);
    initPumila3Module(m);
    initPumila4Module(m);
    initPumila5Module(m);
}
