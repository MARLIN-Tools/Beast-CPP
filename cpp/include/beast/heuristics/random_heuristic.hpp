#pragma once

#include <random>

#include "beast/heuristics/infra/heuristic.hpp"

namespace beast::heuristics {

class RandomHeuristic final : public Heuristic {
   public:
    RandomHeuristic();

   protected:
    [[nodiscard]] double evaluateInternal(const chess::Board& board) const override;

   private:
    mutable std::mt19937 rng_;
};

}  // namespace beast::heuristics

