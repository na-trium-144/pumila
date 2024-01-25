#include <pumila/pumila.h>

int main(int argc, char const *argv[]) {
    auto sim = std::make_shared<pumila::GameSim>();
    pumila::Window window(sim);
    window.loop();
    return 0;
}