#include <pumila/pumila.h>
#include <CLI/CLI.hpp>
#include <vector>
#include <string>

int main(int argc, char const *argv[]) {
    CLI::App app{"pumila-sim"};

    std::vector<std::string> models = {
        "player",
        "pumila3",
        "pumila5",
    };
    for (int i = 0; i < 7; i++) {
        models.push_back("pumila6_" + std::to_string(i));
    }
    for (int i = 0; i < 7; i++) {
        models.push_back("pumila7_" + std::to_string(i));
    }
    for (int i = 200; i <= 800; i += 200) {
        models.push_back("pumila8_" + std::to_string(i));
    }
    for (int i : std::array<int, 5>{100, 316, 1000, 3162, 10000}) {
        models.push_back("pumila6r_" + std::to_string(i));
    }
    std::vector<std::string> selected_models;
    app.add_option("model", selected_models, "Select Player or AI Model")
        ->required()
        ->expected(1, 2)
        ->check(CLI::IsMember(models, CLI::ignore_case));

    CLI11_PARSE(app, argc, argv);

    auto seed = std::random_device()();

    std::vector<std::shared_ptr<pumila::GameSim>> sim = {};
    for (std::size_t i = 0; i < selected_models.size(); i++) {
        if (selected_models[i] == "pumila3") {
            auto model = std::make_shared<pumila::Pumila3>(0.01);
            model->loadFile();
            sim.push_back(std::make_shared<pumila::GameSim>(model, "", seed));
        } else if (selected_models[i] == "pumila5") {
            auto model = std::make_shared<pumila::Pumila5>(0.01);
            model->loadFile();
            sim.push_back(std::make_shared<pumila::GameSim>(model, "", seed));
        } else if (selected_models[i].starts_with("pumila6r")) {
            auto model = std::make_shared<pumila::Pumila6r>(1);
            model->loadFile(selected_models[i]);
            sim.push_back(std::make_shared<pumila::GameSim>(
                model, selected_models[i], seed));
        } else if (selected_models[i].starts_with("pumila6")) {
            auto model = std::make_shared<pumila::Pumila6>(1);
            model->loadFile(selected_models[i]);
            sim.push_back(std::make_shared<pumila::GameSim>(
                model, selected_models[i], seed));
        } else if (selected_models[i].starts_with("pumila7")) {
            auto model = std::make_shared<pumila::Pumila7>(1);
            model->loadFile(selected_models[i]);
            sim.push_back(std::make_shared<pumila::GameSim>(
                model, selected_models[i], seed));
        } else if (selected_models[i].starts_with("pumila8")) {
            auto model = std::make_shared<pumila::Pumila8>(1);
            model->loadFile(selected_models[i]);
            sim.push_back(std::make_shared<pumila::GameSim>(
                model, selected_models[i], seed));
        } else {
            sim.push_back(std::make_shared<pumila::GameSim>(seed));
        }
    }

    pumila::Window window(sim);
    while (true) {
        window.step(true);
    }
    return 0;
}