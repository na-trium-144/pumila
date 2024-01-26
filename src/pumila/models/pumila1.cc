#include <pumila/pumila.h>

namespace pumila {
Pumila1::Pumila1(double alpha, double learning_rate)
    : matrix_ih(Eigen::MatrixXd::Random(HIDDEN_NODES, IN_NODES)),
      matrix_hq(Eigen::MatrixXd::Random(Q_NODES, HIDDEN_NODES)),
      alpha(alpha),
      learning_rate(learning_rate) {}

double Pumila1::forward(const FieldState &fs) {
    in = Eigen::VectorXd(IN_NODES);
    InNodes &in_nodes = *reinterpret_cast<InNodes *>(in.data());
    in_nodes.bias = 1;
    auto chain_all = fs.calcChainAll();
    for (int y = 0; y < FieldState::HEIGHT; y++) {
        for (int x = 0; x < FieldState::WIDTH; x++) {
            int p;
            switch (fs.get(x, y)) {
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
                in_nodes.field_colors[(y * FieldState::WIDTH + x) * 4 + p] = 1;
            }
            in_nodes.field_chains[y * FieldState::WIDTH + x] = chain_all[y][x];
        }
    }

    hidden = matrix_ih * in;
    hidden(0) = 1;                                                // bias
    hidden = (1 / (1 + (hidden.array() * alpha).exp())).matrix(); // sigmoid
    q = (matrix_hq * hidden)(0, 0);
    return q;
}

void Pumila1::backward(double target) {
    auto delta_2 = q - target;
    auto delta_1 = ((delta_2 * matrix_hq).transpose().array() * alpha *
                    hidden.array() * (1 - hidden.array()))
                       .matrix();
    matrix_hq -= learning_rate * hidden.transpose() * delta_2;
    matrix_ih -= learning_rate * delta_1 * in.transpose();
}

} // namespace pumila
