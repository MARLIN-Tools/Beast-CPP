#include "beast/infra/search_options.hpp"

#include <algorithm>
#include <cctype>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <thread>

#include "beast/infra/constants.hpp"

namespace beast::infra {

namespace {

std::string toLower(std::string value) {
    std::transform(
        value.begin(),
        value.end(),
        value.begin(),
        [](const unsigned char ch) { return static_cast<char>(std::tolower(ch)); }
    );
    return value;
}

std::optional<std::size_t> findToken(const std::vector<std::string>& args, const std::string& token) {
    const auto it = std::find(args.begin(), args.end(), token);
    if (it == args.end()) {
        return std::nullopt;
    }
    return static_cast<std::size_t>(it - args.begin());
}

}  // namespace

SearchOptions::SearchOptions() {
    reset();
}

std::vector<std::string> SearchOptions::getUciOptions() {
    SearchOptions options;
    const unsigned int cpu_count = std::max(1u, std::thread::hardware_concurrency());
    return {
        "option name Heuristic type combo default "
            + beast::heuristics::heuristicTypeToString(options.heuristic_type)
            + " var classical var neural_network var random",
        "option name ModelFile type string default <empty>",
        std::string("option name Syzygy50MoveRule type check default ")
            + (options.fifty_moves_rule ? "true" : "false"),
        "option name SyzygyPath type string default <empty>",
        "option name SyzygyProbeLimit type spin default " + std::to_string(options.syzygy_probe_limit)
            + " min 0 max 7",
        "option name Threads type spin default " + std::to_string(options.threads) + " min 1 max "
            + std::to_string(cpu_count),
    };
}

void SearchOptions::reset() {
    board = chess::Board();
    fifty_moves_rule = true;
    heuristic_type = beast::heuristics::HeuristicType::Classical;
    has_model_file = false;
    model_file.clear();
    has_syzygy_path = false;
    syzygy_path.clear();
    syzygy_probe_limit = 7;
    threads = 1;
    resetTemporaryParameters();
}

void SearchOptions::setPosition(const std::vector<std::string>& args) {
    board = chess::Board();
    if (args.empty()) {
        return;
    }

    const auto moves_index = findToken(args, "moves");
    if (args[0] == "fen" && !moves_index.has_value()) {
        const std::string fen = joinTokens(args, 1, args.size());
        if (!fen.empty()) {
            board.setFen(fen);
        }
        return;
    }

    if (!moves_index.has_value()) {
        return;
    }

    if (args[0] == "fen") {
        const std::string fen = joinTokens(args, 1, *moves_index);
        if (!fen.empty()) {
            board.setFen(fen);
        }
    }

    for (std::size_t i = *moves_index + 1; i < args.size(); ++i) {
        const auto move = chess::uci::uciToMove(board, args[i]);
        if (move == chess::Move::NO_MOVE) {
            continue;
        }
        board.makeMove(move);
    }
}

void SearchOptions::setSearchParameters(const std::vector<std::string>& args) {
    resetTemporaryParameters();

    if (args.empty()) {
        depth = Constants::DEFAULT_DEPTH;
        return;
    }

    if (const auto movetime_index = findToken(args, "movetime"); movetime_index.has_value()
        && *movetime_index + 1 < args.size()) {
        move_time = std::stoi(args[*movetime_index + 1]);
    }
    if (const auto wtime_index = findToken(args, "wtime"); wtime_index.has_value()
        && *wtime_index + 1 < args.size()) {
        white_time = std::stoi(args[*wtime_index + 1]);
    }
    if (const auto winc_index = findToken(args, "winc"); winc_index.has_value()
        && *winc_index + 1 < args.size()) {
        white_increment = std::stoi(args[*winc_index + 1]);
    }
    if (const auto btime_index = findToken(args, "btime"); btime_index.has_value()
        && *btime_index + 1 < args.size()) {
        black_time = std::stoi(args[*btime_index + 1]);
    }
    if (const auto binc_index = findToken(args, "binc"); binc_index.has_value()
        && *binc_index + 1 < args.size()) {
        black_increment = std::stoi(args[*binc_index + 1]);
    }
    if (const auto depth_index = findToken(args, "depth"); depth_index.has_value()
        && *depth_index + 1 < args.size()) {
        depth = std::stoi(args[*depth_index + 1]);
    }
    if (findToken(args, "infinite").has_value()) {
        depth = Constants::INFINITE_DEPTH;
    }
}

void SearchOptions::setOption(const std::vector<std::string>& args) {
    if (args.size() < 4) {
        return;
    }

    const std::string option_name = toLower(args[1]);
    const std::string value = joinTokens(args, 3, args.size());

    if (option_name == "syzygy50moverule") {
        const std::string lower = toLower(value);
        if (lower == "true") {
            fifty_moves_rule = true;
        } else if (lower == "false") {
            fifty_moves_rule = false;
        } else {
            std::cout << "Invalid syzygy 50 move rule." << std::endl;
        }
        return;
    }

    if (option_name == "heuristic") {
        try {
            heuristic_type = beast::heuristics::heuristicTypeFromString(toLower(value));
        } catch (const std::runtime_error& err) {
            std::cout << err.what() << std::endl;
        }
        return;
    }

    if (option_name == "modelfile") {
        const std::filesystem::path path(value);
        if (!std::filesystem::exists(path)) {
            std::cout << "Invalid model file." << std::endl;
            return;
        }
        model_file = path;
        has_model_file = true;
        return;
    }

    if (option_name == "syzygypath") {
        const std::filesystem::path path(value);
        if (!std::filesystem::exists(path)) {
            std::cout << "Invalid syzygy path." << std::endl;
            return;
        }
        syzygy_path = path;
        has_syzygy_path = true;
        return;
    }

    if (option_name == "syzygyprobelimit") {
        try {
            syzygy_probe_limit = std::stoi(value);
        } catch (const std::invalid_argument&) {
            std::cout << "Invalid syzygy probe limit." << std::endl;
        }
        return;
    }

    if (option_name == "threads") {
        try {
            threads = std::stoi(value);
        } catch (const std::invalid_argument&) {
            std::cout << "Invalid thread limit." << std::endl;
        }
    }
}

void SearchOptions::resetTemporaryParameters() {
    move_time = 0;
    white_time = 0;
    white_increment = 0;
    black_time = 0;
    black_increment = 0;
    depth = Constants::INFINITE_DEPTH;
}

std::string SearchOptions::joinTokens(
    const std::vector<std::string>& tokens, const std::size_t start, const std::size_t end_exclusive
) {
    if (start >= tokens.size() || start >= end_exclusive) {
        return "";
    }

    std::string result;
    for (std::size_t i = start; i < std::min(tokens.size(), end_exclusive); ++i) {
        if (!result.empty()) {
            result += " ";
        }
        result += tokens[i];
    }
    return result;
}

}  // namespace beast::infra
