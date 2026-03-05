#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include "chess.hpp"

#include "beast/heuristics/infra/heuristic_type.hpp"

namespace beast::infra {

class SearchOptions {
   public:
    SearchOptions();

    [[nodiscard]] static std::vector<std::string> getUciOptions();

    void reset();
    void setPosition(const std::vector<std::string>& args);
    void setSearchParameters(const std::vector<std::string>& args);
    void setOption(const std::vector<std::string>& args);
    void resetTemporaryParameters();

    chess::Board board;

    int move_time;         // [ms]
    int white_time;        // [ms]
    int white_increment;   // [ms]
    int black_time;        // [ms]
    int black_increment;   // [ms]
    int depth;

    bool fifty_moves_rule;
    beast::heuristics::HeuristicType heuristic_type;
    std::filesystem::path model_file;
    bool has_model_file;
    std::filesystem::path syzygy_path;
    bool has_syzygy_path;
    int syzygy_probe_limit;
    int threads;

   private:
    static std::string joinTokens(
        const std::vector<std::string>& tokens, std::size_t start, std::size_t end_exclusive
    );
};

}  // namespace beast::infra

