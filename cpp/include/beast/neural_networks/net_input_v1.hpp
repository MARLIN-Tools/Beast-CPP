#pragma once

#include <vector>

#include "chess.hpp"

namespace beast::neural_networks {

void boardToInputV1(const chess::Board& board, std::vector<float>& input);

}  // namespace beast::neural_networks
