#include <pumila/chain.h>
#include <numeric>
#include <algorithm>

namespace PUMILA_NS {
void Chain::push_connection(Puyo p, int n) {
    if (connection_num >= connections.size()) {
        throw std::out_of_range("number of connections overflow");
    }
    connections[connection_num++] = std::make_pair(p, n);
}

int Chain::connectionNum() const {
    return std::accumulate(
        connections.begin(), connections.begin() + connection_num, 0,
        [](int acc, const auto &con) { return acc + con.second; });
}
int Chain::chainBonus(int chain_num) {
    if (chain_num < 4) {
        return 8 * (chain_num - 1);
    } else {
        return 32 * (chain_num - 3);
    }
}
int Chain::connectionBonus() const {
    int b = 0;
    for (std::size_t i = 0; i < connection_num; i++) {
        int c = connections[i].second;
        if (c >= 5 && c <= 10) {
            b += c - 3;
        } else if (c > 10) {
            b += 10;
        }
    }
    return b;
}
int Chain::colorBonus() const {
    std::array<bool, 6> colors = {};
    for (std::size_t i = 0; i < connection_num; i++) {
        colors[static_cast<int>(connections[i].first)] = true;
    }
    int cn =
        std::count_if(colors.begin(), colors.end(), [](auto c) { return c; });
    if (cn <= 3) {
        return 3 * (cn - 1);
    } else {
        return 12 * (cn - 3);
    }
}
int Chain::scoreA() const { return connectionNum() * 10; }
int Chain::scoreB() const {
    int b = chainBonus() + connectionBonus() + colorBonus();
    return b ? b : 1;
}

} // namespace PUMILA_NS
