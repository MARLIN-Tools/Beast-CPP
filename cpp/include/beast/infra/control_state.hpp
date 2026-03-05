#pragma once

#include <atomic>

namespace beast::infra {

struct ControlState {
    std::atomic<bool> stop{false};
    std::atomic<bool> quit{false};
};

}  // namespace beast::infra

