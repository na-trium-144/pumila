#include <pumila/pumila.h>
#include <CLI/CLI.hpp>
#include <array>
#include <string>

int main(int argc, char const *argv[]) {
    CLI::App app{"pumila-sim"};

    std::array<std::string, 3> models = {
        "player",
        "pumila3",
        "pumila5",
    };
    std::vector<std::string> selected_models;
    app.add_option("model", selected_models, "Select Player or AI Model")
        ->required()
        ->expected(1, 2)
        ->check(CLI::IsMember(models, CLI::ignore_case));

    CLI11_PARSE(app, argc, argv);

    std::vector<std::shared_ptr<pumila::GameSim>> sim = {};
    for (std::size_t i = 0; i < selected_models.size(); i++) {
        if (selected_models[i] == "pumila3") {
            sim.push_back(std::make_shared<pumila::GameSim>(
                std::make_shared<pumila::Pumila3>(0.01)));
        } else if (selected_models[i] == "pumila5") {
            sim.push_back(std::make_shared<pumila::GameSim>(
                std::make_shared<pumila::Pumila5>(0.01)));
        } else {
            sim.push_back(std::make_shared<pumila::GameSim>());
        }
    }

    pumila::Window window(sim);
    while (true) {
        window.step(true);
    }
    return 0;
}