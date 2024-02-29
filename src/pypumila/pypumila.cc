#include <pumila/pumila.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

using namespace PUMILA_NS;
namespace py = pybind11;

void initPumila1Module(py::module_ &m);
void initPumila2Module(py::module_ &m);
void initPumila3Module(py::module_ &m);
void initPumila4Module(py::module_ &m);
void initPumila5Module(py::module_ &m);
void initPumila6Module(py::module_ &m);
void initPumila7Module(py::module_ &m);
void initPumila8Module(py::module_ &m);
void initPumila6rModule(py::module_ &m);
void initPumila7rModule(py::module_ &m);
void initPumila8rModule(py::module_ &m);
void initPumila8sModule(py::module_ &m);
void initPumila9Module(py::module_ &m);
void initPumila10Module(py::module_ &m);
void initPumila11Module(py::module_ &m);
void initPumila12Module(py::module_ &m);

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
    auto fs2 =
        py::class_<FieldState2, std::shared_ptr<FieldState2>>(m, "FieldState2")
            .def(py::init<>())
            .def("field", &FieldState2::field)
            .def("next", py::overload_cast<>(&FieldState2::next, py::const_))
            .def("current_step",
                 py::overload_cast<>(&FieldState2::currentStep, py::const_))
            .def("prev_step", &FieldState2::prevStep)
            .def("last_chain_step", &FieldState2::lastChainStep)
            .def("prev_puyo_num", &FieldState2::prevPuyoNum)
            .def("total_score", &FieldState2::totalScore)
            .def("garbage",
                 py::overload_cast<>(&FieldState2::garbage, py::const_))
            .def("put_next",
                 py::overload_cast<const Action &>(&FieldState2::putNext))
            .def("put_next", py::overload_cast<>(&FieldState2::putNext))
            .def("put_garbage", &FieldState2::putGarbage)
            .def("check_next_collision",
                 py::overload_cast<const Action &>(
                     &FieldState2::checkNextCollision, py::const_))
            .def("check_next_collision",
                 py::overload_cast<>(&FieldState2::checkNextCollision,
                                     py::const_))
            .def("get_next_height",
                 py::overload_cast<const Action &>(&FieldState2::getNextHeight,
                                                   py::const_))
            .def("get_next_height",
                 py::overload_cast<>(&FieldState2::getNextHeight, py::const_))
            .def("get_height", &FieldState2::getHeight)
            .def("delete_chain", &FieldState2::deleteChain)
            .def("delete_chain_recurse", &FieldState2::deleteChainRecurse)
            .def("calc_chain_all", &FieldState2::calcChainAll)
            .def("fall", &FieldState2::fall)
            .def("is_game_over", &FieldState2::isGameOver);
    py::class_<FieldState2::Field>(fs2, "Field")
        .def("set", &FieldState2::Field::set)
        .def("get", &FieldState2::Field::get)
        .def("puyo_num", &FieldState2::Field::puyoNum);
    py::class_<FieldState2::NextList>(fs2, "NextList")
        .def("get", &FieldState2::NextList::get)
        .def("update", &FieldState2::NextList::update)
        .def("pop", &FieldState2::NextList::pop);
    py::class_<FieldState2::StepInfo>(fs2, "StepInfo")
        .def("num", &FieldState2::StepInfo::num)
        .def("chains", &FieldState2::StepInfo::chains)
        .def("chain_num", &FieldState2::StepInfo::chainNum)
        .def("chain_score", &FieldState2::StepInfo::chainScore);
    py::class_<FieldState2::GarbageInfo>(fs2, "GarbageInfo")
        .def("push_score", &FieldState2::GarbageInfo::pushScore)
        .def("add_garbage", &FieldState2::GarbageInfo::addGarbage)
        .def("send", &FieldState2::GarbageInfo::send)
        .def("get_ready", &FieldState2::GarbageInfo::getReady)
        .def("get_current", &FieldState2::GarbageInfo::getCurrent);

    py::class_<FieldState, std::shared_ptr<FieldState>>(m, "FieldState")
        .def(py::init<>())
        .def_readwrite("field", &FieldState::field)
        .def_readwrite("total_score", &FieldState::total_score)
        .def_readwrite("next", &FieldState::next)
        .def_readwrite("prev_chain_num", &FieldState::prev_chain_num)
        .def_readwrite("prev_chain_score", &FieldState::prev_chain_score)
        .def_readwrite("puyo_num", &FieldState::puyo_num)
        .def_readwrite("prev_puyo_num", &FieldState::prev_puyo_num)
        .def_readwrite("is_valid", &FieldState::is_valid)
        .def_readwrite("step_num", &FieldState::step_num)
        .def_readwrite("last_chain_step_num", &FieldState::last_chain_step_num)
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
        .def(py::init<std::shared_ptr<Pumila>>())
        .def("field_copy", &GameSim::field1)
        .def("field2_copy",
             [](std::shared_ptr<GameSim> sim) {
                 std::shared_lock lock(sim->field_m);
                 return std::make_shared<FieldState2>(*sim->field);
             })
        .def_readwrite("model", &GameSim::model)
        .def("has_model", &GameSim::hasModel)
        .def("move_pair", &GameSim::movePair)
        .def("rot_pair", &GameSim::rotPair)
        .def("quick_drop", &GameSim::quickDrop)
        .def("soft_drop", &GameSim::softDrop)
        .def("step", &GameSim::step)
        .def("put", &GameSim::put)
        .def("soft_put", &GameSim::softPut)
        .def("is_free_phase", &GameSim::isFreePhase);
    py::class_<Window>(m, "Window")
        .def(py::init<std::vector<std::shared_ptr<GameSim>>>())
        .def(py::init<std::shared_ptr<GameSim>>())
        .def("step", &Window::step)
        .def("quit", &Window::quit)
        .def("is_running", &Window::isRunning);
    py::class_<Pumila, std::shared_ptr<Pumila>>(m, "Pumila")
        .def("get_action",
             py::overload_cast<const FieldState2 &,
                               const std::optional<FieldState2> &>(
                 &Pumila::getAction))
        .def("get_action", py::overload_cast<const std::shared_ptr<GameSim> &>(
                               &Pumila::getAction))
        .def("action_coeff", &Pumila::actionCoeff)
        .def("load_file", py::overload_cast<>(&Pumila::loadFile))
        .def("load_file", py::overload_cast<std::string>(&Pumila::loadFile))
        .def("save_file", py::overload_cast<>(&Pumila::saveFile))
        .def("save_file", py::overload_cast<std::string>(&Pumila::saveFile));

    initPumila1Module(m);
    initPumila2Module(m);
    initPumila3Module(m);
    initPumila4Module(m);
    initPumila5Module(m);
    initPumila6Module(m);
    initPumila7Module(m);
    initPumila8Module(m);
    initPumila6rModule(m);
    initPumila7rModule(m);
    initPumila8rModule(m);
    initPumila8sModule(m);
    initPumila9Module(m);
    initPumila10Module(m);
    initPumila11Module(m);
    initPumila12Module(m);
}
