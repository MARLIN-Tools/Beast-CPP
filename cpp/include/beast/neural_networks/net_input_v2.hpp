#pragma once

#include <vector>

#include "chess.hpp"

namespace beast::neural_networks {

void boardToInputV2(const chess::Board& board, std::vector<float>& input);

}  // namespace beast::neural_networks
