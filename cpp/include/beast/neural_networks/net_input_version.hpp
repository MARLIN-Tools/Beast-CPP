#pragma once

#include <cctype>
#include <stdexcept>
#include <string>

namespace beast::neural_networks {

enum class NetInputVersion {
    V1,
    V2,
};

inline NetInputVersion netInputVersionFromString(std::string version) {
    for (char& ch : version) {
        ch = static_cast<char>(::tolower(static_cast<unsigned char>(ch)));
    }
    if (version == "v1") {
        return NetInputVersion::V1;
    }
    if (version == "v2") {
        return NetInputVersion::V2;
    }
    throw std::runtime_error("Invalid version string identifier!");
}

}  // namespace beast::neural_networks
