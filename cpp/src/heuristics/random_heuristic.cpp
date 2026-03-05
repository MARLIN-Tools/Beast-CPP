#include "beast/heuristics/random_heuristic.hpp"

namespace beast::heuristics {

RandomHeuristic::RandomHeuristic()
    : Heuristic(true, {}, 7),
      rng_(std::mt19937(std::random_device{}())) {}

double RandomHeuristic::evaluateInternal(const chess::Board& board) const {
    (void)board;
    std::uniform_int_distribution<int> distribution(static_cast<int>(loss_value), static_cast<int>(win_value));
    return static_cast<double>(distribution(rng_));
}

}  // namespace beast::heuristics

