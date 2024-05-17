#include "pumila/step.h"
#include <pumila/pumila.h>
#include <pybind11/detail/common.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

using namespace PUMILA_NS;
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
    py::class_<GarbageGroup, std::shared_ptr<GarbageGroup>>(m, "GarbageGroup")
        .def(py::init<std::size_t>())
        .def("garbage_num", &GarbageGroup::garbageNum)
        .def("rest_garbage_num", &GarbageGroup::restGarbageNum)
        .def("cancel", &GarbageGroup::cancel)
        .def("fall", &GarbageGroup::fall)
        .def("done", &GarbageGroup::done)
        .def("cancelled_num", &GarbageGroup::cancelledNum)
        .def("fell_num", &GarbageGroup::fellNum);
    py::class_<FieldState3>(m, "FieldState3")
        .def(py::init<>())
        .def("in_range", &FieldState3::inRange)
        .def("get", &FieldState3::get)
        .def("set", &FieldState3::set)
        .def("get_next", &FieldState3::getNext)
        .def("update_next", &FieldState3::updateNext)
        .def("add_garbage", &FieldState3::addGarbage)
        .def("get_garbage_num_total", &FieldState3::getGarbageNumTotal)
        .def("calc_garbage", &FieldState3::calcGarbage)
        .def("cancel_garbage", &FieldState3::cancelGarbage)
        .def("get_next_height", py::overload_cast<const Action &>(
                                    &FieldState3::getNextHeight, py::const_))
        .def("get_next_height",
             py::overload_cast<>(&FieldState3::getNextHeight, py::const_))
        .def("get_height", &FieldState3::getHeight)
        .def("put_next", &FieldState3::putNext)
        .def("delete_chain", &FieldState3::deleteChain)
        .def("fall", &FieldState3::fall)
        .def("delete_chain_recurse", &FieldState3::deleteChainRecurse)
        .def("calc_chain_all", &FieldState3::calcChainAll)
        .def("total_score", &FieldState3::totalScore)
        .def("is_game_over", &FieldState3::isGameOver);
    py::class_<Chain>(m, "Chain")
        .def_readonly("connections", &Chain::connections)
        .def_readonly("chain_num", &Chain::chain_num)
        .def("is_empty", &Chain::isEmpty)
        .def("connection_num", &Chain::connectionNum)
        .def("score", &Chain::score)
        .def("score_a", &Chain::scoreA)
        .def("score_b", &Chain::scoreB);
    py::class_<StepResult, std::shared_ptr<StepResult>>(m, "StepResult")
        .def_readonly("field_before", &StepResult::field_before)
        .def_readonly("field_after", &StepResult::field_after)
        .def_readonly("chains", &StepResult::chains)
        .def_readonly("garbage_send", &StepResult::garbage_send)
        .def_readonly("op_field_before", &StepResult::op_field_before)
        .def_readonly("op_field_after", &StepResult::op_field_after)
        .def_readonly("garbage_recv", &StepResult::garbage_recv)
        .def_readonly("garbage_fell_pos", &StepResult::garbage_fell_pos)
        .def("done", &StepResult::done)
        .def("next", &StepResult::next);
    auto game_sim =
        py::class_<GameSim, std::shared_ptr<GameSim>>(m, "GameSim")
            .def("make_new",
                 py::overload_cast<typename std::mt19937::result_type, bool>(
                     &GameSim::makeNew))
            .def("make_new", py::overload_cast<>(&GameSim::makeNew))
            .def("field_copy", [](const GameSim &sim) { return sim.field; })
            .def("current_step",
                 [](const GameSim &sim) { return sim.current_step; })
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
            .def("phase_get",
                 [](const GameSim &sim) {
                     return sim.phase ? sim.phase->get() : GameSim::Phase::none;
                 })
            .def("fall_phase_current_chain",
                 [](const GameSim &sim) -> std::size_t {
                     auto fall_phase =
                         dynamic_cast<GameSim::FallPhase *>(sim.phase.get());
                     if (fall_phase) {
                         return fall_phase->current_chain;
                     } else {
                         return 0;
                     }
                 })
            .def("fall_phase_display_field",
                 [](const GameSim &sim) -> std::optional<FieldState3> {
                     auto fall_phase =
                         dynamic_cast<GameSim::FallPhase *>(sim.phase.get());
                     if (fall_phase) {
                         return fall_phase->display_field;
                     } else {
                         return std::nullopt;
                     }
                 });
    py::enum_<GameSim::Phase::PhaseEnum>(game_sim, "PhaseEnum")
        .value("none", GameSim::Phase::none)
        .value("garbage", GameSim::Phase::garbage)
        .value("free", GameSim::Phase::free)
        .value("fall", GameSim::Phase::fall)
        .export_values();
    py::class_<Matrix>(m, "Matrix", py::buffer_protocol())
        .def_buffer([](Matrix &m) -> py::buffer_info {
            return py::buffer_info(m.ptr(), sizeof(double),
                                   py::format_descriptor<double>::format(), 2,
                                   {m.rows(), m.cols()},
                                   {sizeof(double) * m.cols(), sizeof(double)});
        })
        .def(py::init([](py::buffer b) {
            /* Request a buffer descriptor from Python */
            py::buffer_info info = b.request();

            /* Some basic validation checks ... */
            if (info.format != py::format_descriptor<double>::format()) {
                throw std::runtime_error(
                    "Incompatible format: expected a double array!");
            }
            if (info.ndim != 2) {
                throw std::runtime_error("Incompatible buffer dimension!");
            }
            auto strides_row = info.strides[0] / (py::ssize_t)sizeof(double);
            auto strides_col = info.strides[1] / (py::ssize_t)sizeof(double);
            auto rows = info.shape[0];
            auto cols = info.shape[1];
            Matrix m(rows, cols);
            for (int y = 0; y < rows; y++) {
                for (int x = 0; x < cols; x++) {
                    m.at(y, x) = static_cast<const double *>(
                        info.ptr)[y * strides_row + x * strides_col];
                }
            }
            return m;
        }));

    py::class_<Pumila14>(m, "Pumila14")
        .def("feature_num", []() { return Pumila14::FEATURE_NUM; })
        .def("calc_action", &Pumila14::calcAction)
        .def("transpose", &Pumila14::transpose)
        .def("reward", &Pumila14::reward);
}
