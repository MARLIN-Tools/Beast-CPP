#pragma once

#include "beast/infra/search_options.hpp"

namespace beast::infra {

struct EngineCommand {
    SearchOptions search_options{};
    bool stop{false};
    bool quit{false};
};

}  // namespace beast::infra

