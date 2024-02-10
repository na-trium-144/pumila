#include <pumila/model_base.h>
#include <fstream>

namespace PUMILA_NS {
void Pumila::loadFile() {
    if (!name().empty()) {
        loadFile(name());
    }
}
void Pumila::loadFile(std::string file_name) {
    if (!file_name.ends_with(".bin")) {
        file_name += ".bin";
    }
    if (file_name != ".bin") {
        std::ifstream ifs(file_name, std::ios_base::in | std::ios_base::binary);
        if (ifs) {
            load(ifs);
        } else {
            std::cerr << "error opening file " + file_name << std::endl;
        }
    }
}
void Pumila::saveFile() {
    if (!name().empty()) {
        saveFile(name());
    }
}
void Pumila::saveFile(std::string file_name) {
    if (!file_name.ends_with(".bin")) {
        file_name += ".bin";
    }
    if (file_name != ".bin") {
        std::ofstream ofs(file_name,
                          std::ios_base::out | std::ios_base::binary);
        if (ofs) {
            save(ofs);
        } else {
            std::cerr << "error opening file " + file_name << std::endl;
        }
    }
}
} // namespace PUMILA_NS
