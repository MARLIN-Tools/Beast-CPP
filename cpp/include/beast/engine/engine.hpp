#pragma once

#include <array>
#include <chrono>
#include <cstdint>
#include <memory>
#include <optional>
#include <random>
#include <utility>
#include <vector>

#include "chess.hpp"

#include "beast/heuristics/infra/heuristic.hpp"
#include "beast/infra/concurrent_queue.hpp"
#include "beast/infra/control_state.hpp"
#include "beast/infra/engine_command.hpp"
#include "beast/infra/search_options.hpp"

namespace beast::engine {

class Engine {
   public:
    Engine(
        beast::infra::ConcurrentQueue<beast::infra::EngineCommand>& queue,
        beast::infra::ControlState& control
    );

    void start();

   private:
    struct HeuristicKey {
        beast::heuristics::HeuristicType heuristic_type;
        std::string model_file;
        bool fifty_moves_rule;
        std::string syzygy_path;
        int syzygy_probe_limit;
        int threads;

        [[nodiscard]] bool operator==(const HeuristicKey& rhs) const {
            return heuristic_type == rhs.heuristic_type && model_file == rhs.model_file
                && fifty_moves_rule == rhs.fifty_moves_rule && syzygy_path == rhs.syzygy_path
                && syzygy_probe_limit == rhs.syzygy_probe_limit && threads == rhs.threads;
        }
    };

    struct TTEntry {
        std::uint64_t key{0};
        int depth{-1};
        double score{0.0};
        std::uint8_t flag{0};  // 0 exact, 1 lower, 2 upper
        std::uint16_t best_move{chess::Move::NO_MOVE};
    };

    static constexpr std::uint64_t COMMAND_CHECK_INTERVAL = 2048;
    static constexpr int MAX_PLY = 128;
    static constexpr std::size_t TT_SIZE = 1U << 20;

    void checkStop();
    beast::heuristics::Heuristic& chooseHeuristic(beast::infra::SearchOptions& search_options);
    void startTimer(const beast::infra::SearchOptions& search_options);
    void clearTimer();
    void search(chess::Board board, int max_depth);
    std::pair<double, std::vector<chess::Move>>
    negamax(chess::Board& board, int depth, double alpha, double beta, int ply);
    double quiescence(chess::Board& board, double alpha, double beta, int ply);

    std::vector<chess::Move> getCaptures(const chess::Board& board) const;
    std::vector<chess::Move> orderMoves(
        const chess::Board& board,
        const std::vector<chess::Move>& moves,
        chess::Move tt_move,
        int ply
    ) const;

    std::unique_ptr<beast::heuristics::Heuristic> heuristic_;
    std::optional<HeuristicKey> heuristic_key_;
    std::uint64_t nodes_searched_;
    beast::infra::ConcurrentQueue<beast::infra::EngineCommand>& queue_;
    beast::infra::ControlState& control_;
    std::optional<std::chrono::steady_clock::time_point> deadline_;
    std::mt19937 rng_;
    int selective_depth_;
    std::vector<TTEntry> tt_;
    std::array<std::array<chess::Move, 2>, MAX_PLY> killers_;
    std::array<std::array<std::array<int, 64>, 64>, 2> history_;
};

}  // namespace beast::engine
