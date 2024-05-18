#include <cstddef>
#include <pumila/action.h>
#include <pumila/models/pumila14.h>
#include <pumila/models/common.h>

namespace PUMILA_NS {
void calcActionEach(Pumila14::InFeature *feat, FieldState3 field_copy, int a) {
    field_copy.updateNext({field_copy.getNext(0), actions[a]});
    bool action_in_field = field_copy.putNext();
    auto chains = field_copy.deleteChainRecurse();

    // feat->bias = 1;
    auto chain_all = field_copy.calcChainAll();
    for (std::size_t y = 0; y < FieldState3::HEIGHT; y++) {
        for (std::size_t x = 0; x < FieldState3::WIDTH; x++) {
            int p;
            switch (field_copy.get(x, y)) {
            case Puyo::red:
                p = 0;
                break;
            case Puyo::blue:
                p = 1;
                break;
            case Puyo::green:
                p = 2;
                break;
            case Puyo::yellow:
                p = 3;
                break;
            default:
                p = -1;
                break;
            }
            if (p >= 0) {
                feat->field_colors[(y * FieldState3::WIDTH + x) * 4 + p] = 1;
                feat->field_chains[(y * FieldState3::WIDTH + x) * 4 + p] =
                    chain_all[y][x];
            }
        }
    }
    for (std::size_t i = 0;
         i < chains.size() && i < sizeof(feat->score_diff) / sizeof(double);
         i++) {
        double score_base = 40.0 * Chain::chainBonus(i + 1);
        feat->score_diff[i] =
            chains[i].score() / (score_base ? score_base : 40.0);
    }
}

Matrix Pumila14::calcAction(const StepResult &result) {
    Matrix m(ACTIONS_NUM, FEATURE_NUM);
    std::array<std::future<void>, ACTIONS_NUM> tasks;
    for (int a = 0; a < ACTIONS_NUM; a++) {
        auto m_ptr = m.rowPtr<InFeature>(a);
        tasks[a] = pool.submit_task([m_ptr, &result, a] {
            calcActionEach(m_ptr, result.field_before, a);
        });
    }
    for (int a = 0; a < ACTIONS_NUM; a++) {
        tasks[a].get();
    }
    return m;
}

Matrix Pumila14::rotateColor(const Matrix &in) {
    assert(in.cols() == sizeof(InFeature) / sizeof(double) &&
           "invalid size in Pumila14::rotateColor");
    Matrix ret(in.rows() * 24, sizeof(InFeature) / sizeof(double));
    std::array<int, 4> index = {0, 1, 2, 3};
    int r = 0;
    do {
        assert(r < 24);
        for (std::size_t a = 0; a < in.rows(); a++) {
            auto in_ptr = in.rowPtr<InFeature>(a);
            auto ret_ptr = ret.rowPtr<InFeature>(a + r * in.rows());
            // ret_ptr->bias = 1;
            for (std::size_t p = 0;
                 p < FieldState3::WIDTH * FieldState3::HEIGHT * 4; p += 4) {
                for (int i = 0; i < 4; i++) {
                    ret_ptr->field_colors[p + index[i]] =
                        in_ptr->field_colors[p + i];
                    ret_ptr->field_chains[p + index[i]] =
                        in_ptr->field_chains[p + i];
                }
            }
            for (std::size_t i = 0;
                 i < sizeof(InFeature::score_diff) / sizeof(double); i++) {
                ret_ptr->score_diff[i] = in_ptr->score_diff[i];
            }
        }
        r++;
    } while (std::next_permutation(index.begin(), index.end()));
    return ret;
}

double Pumila14::reward(const StepResult &result) {
    assert(result.done());
    return std::accumulate(
        result.chains.cbegin(), result.chains.end(), 0,
        [](int acc, const Chain &c) { return acc + c.score(); });
}

} // namespace PUMILA_NS
