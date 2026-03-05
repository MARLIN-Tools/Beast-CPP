#pragma once

#include <filesystem>

#include "chess.hpp"

namespace beast::heuristics {

class Heuristic {
   public:
    Heuristic(bool fifty_moves_rule, std::filesystem::path syzygy_path, int syzygy_probe_limit);
    virtual ~Heuristic() = default;

    [[nodiscard]] double evaluateResult(const chess::Board& board, int depth) const;
    [[nodiscard]] double evaluatePosition(const chess::Board& board, bool check_game_over = true) const;
    [[nodiscard]] virtual bool useQuiescence() const;

    static double centipawnToProbability(int centipawn);
    static double probabilityToCentipawn(double win_probability);

    double draw_value;
    double loss_value;
    double win_value;

   protected:
    [[nodiscard]] virtual double evaluateInternal(const chess::Board& board) const = 0;

    bool fifty_moves_rule_;
    std::filesystem::path syzygy_path_;
    int syzygy_probe_limit_;
};

}  // namespace beast::heuristics

