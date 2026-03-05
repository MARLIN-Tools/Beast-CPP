#include "beast/heuristics/infra/heuristic.hpp"

#include <cmath>
#include <utility>

namespace beast::heuristics {

Heuristic::Heuristic(
    const bool fifty_moves_rule, std::filesystem::path syzygy_path, const int syzygy_probe_limit
)
    : fifty_moves_rule_(fifty_moves_rule),
      syzygy_path_(std::move(syzygy_path)),
      syzygy_probe_limit_(syzygy_probe_limit),
      draw_value(probabilityToCentipawn(0.5) * 100.0),
      loss_value(probabilityToCentipawn(0.0) * 100.0),
      win_value(probabilityToCentipawn(1.0) * 100.0) {}

double Heuristic::evaluateResult(const chess::Board& board, const int depth) const {
    const auto [reason, result] = board.isGameOver();
    (void)reason;
    if (result == chess::GameResult::DRAW) {
        return draw_value;
    }
    if (result == chess::GameResult::LOSE) {
        return loss_value - 100.0 * static_cast<double>(depth);
    }
    return win_value + 100.0 * static_cast<double>(depth);
}

double Heuristic::evaluatePosition(const chess::Board& board, const bool check_game_over) const {
    if (check_game_over) {
        const auto [reason, result] = board.isGameOver();
        if (reason != chess::GameResultReason::NONE) {
            if (result == chess::GameResult::LOSE) {
                return loss_value;
            }
            return draw_value;
        }
    }

    // Syzygy probing is not implemented in this C++ port yet.
    return evaluateInternal(board);
}

bool Heuristic::useQuiescence() const {
    return false;
}

double Heuristic::centipawnToProbability(const int centipawn) {
    return 1.0 / (1.0 + std::pow(10.0, -(static_cast<double>(centipawn) / 100.0) / 4.0));
}

double Heuristic::probabilityToCentipawn(double win_probability) {
    if (win_probability <= 0.0) {
        win_probability = 1e-9;
    } else if (win_probability >= 1.0) {
        win_probability = 1.0 - 1e-9;
    }
    return 4.0 * std::log10(win_probability / (1.0 - win_probability)) * 100.0;
}

}  // namespace beast::heuristics
