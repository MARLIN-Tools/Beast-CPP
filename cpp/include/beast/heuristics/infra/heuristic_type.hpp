#pragma once

#include <stdexcept>
#include <string>

namespace beast::heuristics {

enum class HeuristicType {
    Classical,
    NeuralNetwork,
    Random,
};

inline HeuristicType heuristicTypeFromString(const std::string& value) {
    if (value == "classical") {
        return HeuristicType::Classical;
    }
    if (value == "neural_network") {
        return HeuristicType::NeuralNetwork;
    }
    if (value == "random") {
        return HeuristicType::Random;
    }
    throw std::runtime_error("Invalid heuristic type identifier!");
}

inline std::string heuristicTypeToString(HeuristicType value) {
    switch (value) {
        case HeuristicType::Classical:
            return "classical";
        case HeuristicType::NeuralNetwork:
            return "neural_network";
        case HeuristicType::Random:
            return "random";
    }
    throw std::runtime_error("Unsupported heuristic type.");
}

}  // namespace beast::heuristics

