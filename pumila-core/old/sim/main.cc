#include <pumila/pumila.h>
#include <CLI/CLI.hpp>
#include <vector>
#include <string>

int main(int argc, char const *argv[]) {
    using namespace std::string_literals;
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
    for (int i : std::array<int, 5>{100, 316, 1000, 3162, 10000}) {
        models.push_back("pumila7r_" + std::to_string(i));
    }
    for (int i : std::array<int, 5>{100, 316, 1000, 3162, 10000}) {
        models.push_back("pumila8s_" + std::to_string(i));
    }
    models.push_back("pumila9_" + std::to_string(300));
    for (const std::string &i :
         std::array<std::string, 4>{"0.5", "0.75", "0.9", "0.99"}) {
        models.push_back("pumila11_300_"s + i + "_100000"s);
    }

    std::vector<std::string> selected_models;
    app.add_option("model", selected_models, "Select Player or AI Model")
        ->required()
        ->expected(1, 2)
        ->check(CLI::IsMember(models, CLI::ignore_case));

    bool single = false;
    app.add_flag("-s,--single", single,
                 "Single Player Mode (Disable Garbage Puyo)");

    CLI11_PARSE(app, argc, argv);

    auto seed = std::random_device()();

    std::vector<std::shared_ptr<pumila::GameSim>> sim = {};
    for (std::size_t i = 0; i < selected_models.size(); i++) {
        std::shared_ptr<pumila::Pumila> model = nullptr;
        if (selected_models[i] == "pumila3") {
            model = std::make_shared<pumila::Pumila3>(0.01);
            model->loadFile();
        } else if (selected_models[i] == "pumila5") {
            model = std::make_shared<pumila::Pumila5>(0.01);
            model->loadFile();
        } else if (selected_models[i].starts_with("pumila6r")) {
            model = std::make_shared<pumila::Pumila6r>(1);
            model->loadFile(selected_models[i]);
        } else if (selected_models[i].starts_with("pumila6")) {
            model = std::make_shared<pumila::Pumila6>(1);
            model->loadFile(selected_models[i]);
        } else if (selected_models[i].starts_with("pumila7r")) {
            model = std::make_shared<pumila::Pumila7r>(1);
            model->loadFile(selected_models[i]);
        } else if (selected_models[i].starts_with("pumila7")) {
            model = std::make_shared<pumila::Pumila7>(1);
            model->loadFile(selected_models[i]);
        } else if (selected_models[i].starts_with("pumila8s")) {
            model = std::make_shared<pumila::Pumila8s>(1);
            model->loadFile(selected_models[i]);
        } else if (selected_models[i].starts_with("pumila8")) {
            model = std::make_shared<pumila::Pumila8>(1);
            model->loadFile(selected_models[i]);
        } else if (selected_models[i].starts_with("pumila9")) {
            model = std::make_shared<pumila::Pumila9>(1);
            model->loadFile(selected_models[i]);
        } else if (selected_models[i].starts_with("pumila11")) {
            model = std::make_shared<pumila::Pumila11>(1, 1);
            model->loadFile(selected_models[i]);
        }
        sim.push_back(std::make_shared<pumila::GameSim>(
            model, selected_models[i], seed, !single));
    }
    if (sim.size() == 2) {
        sim[0]->opponent = sim[1];
        sim[1]->opponent = sim[0];
    }

    pumila::Window window(sim);
    while (window.isRunning()) {
        window.step(true);
    }
    return 0;
}
