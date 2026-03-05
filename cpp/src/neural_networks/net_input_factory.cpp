#include "beast/neural_networks/net_input_factory.hpp"

#include "beast/neural_networks/net_input_v1.hpp"
#include "beast/neural_networks/net_input_v2.hpp"

namespace beast::neural_networks {

NetInputFunction netInputFromVersion(const NetInputVersion version) {
    switch (version) {
        case NetInputVersion::V1:
            return boardToInputV1;
        case NetInputVersion::V2:
            return boardToInputV2;
    }
    return boardToInputV1;
}

NetInputFunction netInputFromString(const std::string& version) {
    return netInputFromVersion(netInputVersionFromString(version));
}

}  // namespace beast::neural_networks
