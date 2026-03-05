#pragma once

#include <string>
#include <vector>

#include "beast/infra/concurrent_queue.hpp"
#include "beast/infra/control_state.hpp"
#include "beast/infra/engine_command.hpp"
#include "beast/infra/search_options.hpp"

namespace beast::engine {

class UciProtocol {
   public:
    UciProtocol(
        beast::infra::ConcurrentQueue<beast::infra::EngineCommand>& queue,
        beast::infra::ControlState& control
    );

    void uciLoop();

   private:
    static std::vector<std::string> tokenize(const std::string& line);

    void uci() const;
    static void isReady();
    void quit();
    void go(const std::vector<std::string>& args);
    void stop();
    void setOption(const std::vector<std::string>& args);
    void newGame();
    void position(const std::vector<std::string>& args);
    static void invalidCommand(const std::string& command);

    beast::infra::ConcurrentQueue<beast::infra::EngineCommand>& queue_;
    beast::infra::ControlState& control_;
    beast::infra::SearchOptions search_options_;
};

}  // namespace beast::engine

