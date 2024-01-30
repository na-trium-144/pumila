#include <pumila/models/pumila1n.h>
#include <cassert>
#include <stdexcept>
#include <string>

namespace pumila {
int Pumila1N::getAction(const FieldState &field) {
    std::array<std::future<double>, ACTIONS_NUM> actions_result;
    for (int a = 0; a < ACTIONS_NUM; a++) {
        actions_result[a] = pool.submit_task([&field, a, this]() -> double {
            FieldState field_next = field.copy();
            field_next.put(actions[a]);
            std::vector<Chain> chains = field_next.deleteChainRecurse();
            if (chains.size() >= target_chain) {
                return 100;
            }
            auto chain_all = field_next.calcChainAll();
            int max_chain = 0;
            for (int y = 0; y < FieldState::HEIGHT; y++) {
                for (int x = 0; x < FieldState::WIDTH; x++) {
                    max_chain = max_chain > chain_all[y][x] ? max_chain
                                                            : chain_all[y][x];
                }
            }
            std::size_t max_y = 0;
            for (std::size_t x = 0; x < FieldState::WIDTH; x++) {
                auto y = field_next.getHeight(x);
                max_y = y > max_y ? y : max_y;
            }
            return max_chain * 10 - static_cast<int>(max_y);
        });
    }
    int max_sc = -1, max_a = -1;
    for (int a = 0; a < ACTIONS_NUM; a++) {
        int sc = actions_result[a].get();
        if (max_sc < sc) {
            max_sc = sc;
            max_a = a;
        }
    }
    return max_a;
}

} // namespace pumila
