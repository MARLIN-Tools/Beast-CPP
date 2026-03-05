#pragma once

#include <functional>
#include <string>
#include <vector>

#include "chess.hpp"
#include "beast/neural_networks/net_input_version.hpp"

namespace beast::neural_networks {

using NetInputFunction = std::function<void(const chess::Board&, std::vector<float>&)>;

NetInputFunction netInputFromVersion(NetInputVersion version);
NetInputFunction netInputFromString(const std::string& version);

}  // namespace beast::neural_networks
