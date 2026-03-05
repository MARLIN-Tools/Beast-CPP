#include "beast/engine/uci_protocol.hpp"

#include <iostream>
#include <sstream>
#include <utility>

#include "beast/infra/constants.hpp"

namespace beast::engine {

UciProtocol::UciProtocol(
    beast::infra::ConcurrentQueue<beast::infra::EngineCommand>& queue, beast::infra::ControlState& control
)
    : queue_(queue),
      control_(control) {}

void UciProtocol::uciLoop() {
    std::cout << beast::infra::Constants::ENGINE_NAME << " " << beast::infra::Constants::ENGINE_VERSION << " by "
              << beast::infra::Constants::AUTHOR << std::endl;

    std::string line;
    while (std::getline(std::cin, line)) {
        const auto command_tokens = tokenize(line);
        if (command_tokens.empty()) {
            continue;
        }

        const std::string command = command_tokens.front();
        std::vector<std::string> args(command_tokens.begin() + 1, command_tokens.end());

        if (command == "uci") {
            uci();
        } else if (command == "isready") {
            isReady();
        } else if (command == "go") {
            go(args);
        } else if (command == "stop") {
            stop();
        } else if (command == "setoption") {
            setOption(args);
        } else if (command == "ucinewgame") {
            newGame();
        } else if (command == "position") {
            position(args);
        } else if (command == "quit") {
            quit();
            break;
        } else {
            invalidCommand(command);
        }
    }

    if (!control_.quit.load()) {
        quit();
    }
}

std::vector<std::string> UciProtocol::tokenize(const std::string& line) {
    std::vector<std::string> tokens;
    std::istringstream stream(line);
    std::string token;
    while (stream >> token) {
        tokens.push_back(token);
    }
    return tokens;
}

void UciProtocol::uci() const {
    std::cout << "id name " << beast::infra::Constants::ENGINE_NAME << " " << beast::infra::Constants::ENGINE_VERSION
              << std::endl;
    std::cout << "id author " << beast::infra::Constants::AUTHOR << std::endl;
    for (const auto& option : beast::infra::SearchOptions::getUciOptions()) {
        std::cout << option << std::endl;
    }
    std::cout << "uciok" << std::endl;
}

void UciProtocol::isReady() {
    std::cout << "readyok" << std::endl;
}

void UciProtocol::quit() {
    control_.stop.store(true);
    control_.quit.store(true);
    beast::infra::EngineCommand command;
    command.quit = true;
    queue_.push(std::move(command));
}

void UciProtocol::go(const std::vector<std::string>& args) {
    search_options_.setSearchParameters(args);
    control_.stop.store(false);
    beast::infra::EngineCommand command;
    command.search_options = search_options_;
    queue_.push(std::move(command));
    search_options_.resetTemporaryParameters();
}

void UciProtocol::stop() {
    control_.stop.store(true);
}

void UciProtocol::setOption(const std::vector<std::string>& args) {
    search_options_.setOption(args);
}

void UciProtocol::newGame() {
    search_options_.reset();
}

void UciProtocol::position(const std::vector<std::string>& args) {
    search_options_.setPosition(args);
}

void UciProtocol::invalidCommand(const std::string& command) {
    std::cout << "Invalid command: " << command << std::endl;
}

}  // namespace beast::engine
