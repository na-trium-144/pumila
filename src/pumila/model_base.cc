#include <pumila/model_base.h>
#include <fstream>

namespace pumila {
void Pumila::loadFile() {
    if (!name().empty()) {
        std::ifstream ifs(name() + ".bin",
                          std::ios_base::in | std::ios_base::binary);
        if (ifs) {
            load(ifs);
        } else {
            std::cerr << "error opening file " + name() + ".bin" << std::endl;
        }
    }
}
void Pumila::saveFile() {
    if (!name().empty()) {
        std::ofstream ofs(name() + ".bin",
                          std::ios_base::out | std::ios_base::binary);
        if (ofs) {
            save(ofs);
        } else {
            std::cerr << "error opening file " + name() + ".bin" << std::endl;
        }
    }
}
} // namespace pumila
