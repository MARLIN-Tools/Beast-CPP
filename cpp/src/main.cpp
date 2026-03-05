#include <thread>

#include "beast/engine/engine.hpp"
#include "beast/engine/uci_protocol.hpp"
#include "beast/infra/concurrent_queue.hpp"
#include "beast/infra/control_state.hpp"
#include "beast/infra/engine_command.hpp"

int main() {
    beast::infra::ConcurrentQueue<beast::infra::EngineCommand> queue;
    beast::infra::ControlState control;

    beast::engine::Engine engine(queue, control);
    std::thread engine_thread([&engine] { engine.start(); });

    beast::engine::UciProtocol protocol(queue, control);
    protocol.uciLoop();

    if (engine_thread.joinable()) {
        engine_thread.join();
    }

    return 0;
}

