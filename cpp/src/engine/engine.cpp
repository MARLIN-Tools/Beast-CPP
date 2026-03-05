#include "beast/engine/engine.hpp"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <utility>

#include "beast/heuristics/classical_heuristic.hpp"
#include "beast/heuristics/infra/piece_values.hpp"
#include "beast/heuristics/neural_network.hpp"
#include "beast/heuristics/random_heuristic.hpp"
#include "beast/infra/constants.hpp"

namespace beast::engine {
namespace {

constexpr std::uint8_t TT_EXACT = 0;
constexpr std::uint8_t TT_LOWER = 1;
constexpr std::uint8_t TT_UPPER = 2;
constexpr int LMR_MIN_DEPTH = 3;
constexpr int LMR_MIN_MOVE_INDEX = 3;
constexpr int NULL_MOVE_MIN_DEPTH = 4;
constexpr int NULL_MOVE_REDUCTION = 2;
constexpr int NULL_MOVE_MIN_PIECES = 10;
constexpr double NEG_INF = -1'000'000'000.0;
constexpr double POS_INF = 1'000'000'000.0;

}  // namespace

Engine::Engine(
    beast::infra::ConcurrentQueue<beast::infra::EngineCommand>& queue, beast::infra::ControlState& control
)
    : nodes_searched_(0),
      queue_(queue),
      control_(control),
      rng_(std::mt19937(std::random_device{}())),
      selective_depth_(0),
      tt_(TT_SIZE),
      killers_(),
      history_() {}

void Engine::start() {
    while (true) {
        auto command = queue_.pop();
        if (command.quit || control_.quit.load()) {
            break;
        }
        if (command.stop) {
            continue;
        }

        try {
            auto options = command.search_options;
            auto& heuristic = chooseHeuristic(options);
            (void)heuristic;
            startTimer(options);
            search(options.board, options.depth);
            clearTimer();
        } catch (const std::runtime_error& err) {
            clearTimer();
            std::cout << "info string " << err.what() << std::endl;
            std::cout << "bestmove 0000" << std::endl;
        }
    }
}

void Engine::checkStop() {
    if (control_.quit.load()) {
        throw std::runtime_error("Quit command.");
    }
    if (control_.stop.load() && nodes_searched_ % COMMAND_CHECK_INTERVAL == 0) {
        throw std::runtime_error("Stop command.");
    }
    if (deadline_.has_value() && std::chrono::steady_clock::now() >= *deadline_) {
        throw std::runtime_error("Time-out.");
    }
}

beast::heuristics::Heuristic& Engine::chooseHeuristic(beast::infra::SearchOptions& search_options) {
    const HeuristicKey key = {
        search_options.heuristic_type,
        search_options.has_model_file ? search_options.model_file.string() : "",
        search_options.fifty_moves_rule,
        search_options.has_syzygy_path ? search_options.syzygy_path.string() : "",
        search_options.syzygy_probe_limit,
        search_options.threads,
    };

    if (heuristic_ && heuristic_key_.has_value() && heuristic_key_.value() == key) {
        if (search_options.heuristic_type == beast::heuristics::HeuristicType::Random) {
            search_options.depth = 1;
        }
        return *heuristic_;
    }

    if (search_options.heuristic_type == beast::heuristics::HeuristicType::Classical) {
        heuristic_ = std::make_unique<beast::heuristics::ClassicalHeuristic>(
            search_options.fifty_moves_rule,
            search_options.has_syzygy_path ? search_options.syzygy_path : std::filesystem::path(),
            search_options.syzygy_probe_limit
        );
        heuristic_key_ = key;
        return *heuristic_;
    }

    if (search_options.heuristic_type == beast::heuristics::HeuristicType::Random) {
        search_options.depth = 1;
        heuristic_ = std::make_unique<beast::heuristics::RandomHeuristic>();
        heuristic_key_ = key;
        return *heuristic_;
    }

    if (!search_options.has_model_file) {
        throw std::runtime_error("Warning: incorrect model file.");
    }

    try {
        heuristic_ = std::make_unique<beast::heuristics::NeuralNetwork>(
            search_options.model_file,
            search_options.fifty_moves_rule,
            search_options.has_syzygy_path ? search_options.syzygy_path : std::filesystem::path(),
            search_options.syzygy_probe_limit,
            search_options.threads
        );
        heuristic_key_ = key;
        return *heuristic_;
    } catch (const std::runtime_error& err) {
        std::cout << "info string " << err.what() << " Falling back to classical heuristic." << std::endl;
        search_options.heuristic_type = beast::heuristics::HeuristicType::Classical;
        heuristic_ = std::make_unique<beast::heuristics::ClassicalHeuristic>(
            search_options.fifty_moves_rule,
            search_options.has_syzygy_path ? search_options.syzygy_path : std::filesystem::path(),
            search_options.syzygy_probe_limit
        );
        heuristic_key_ = HeuristicKey{
            search_options.heuristic_type,
            "",
            search_options.fifty_moves_rule,
            search_options.has_syzygy_path ? search_options.syzygy_path.string() : "",
            search_options.syzygy_probe_limit,
            search_options.threads,
        };
    }

    return *heuristic_;
}

void Engine::startTimer(const beast::infra::SearchOptions& search_options) {
    clearTimer();

    const bool white_to_move = search_options.board.sideToMove() == chess::Color::WHITE;
    int available_time = 0;
    int time_for_move = 0;

    if (search_options.move_time > 0) {
        available_time = search_options.move_time;
        time_for_move = search_options.move_time;
    } else if (white_to_move && search_options.white_time > 0 && search_options.white_increment == 0) {
        available_time = search_options.white_time - beast::infra::Constants::TIME_FLEX;
        time_for_move = static_cast<int>(0.05 * static_cast<double>(available_time));
    } else if (white_to_move && search_options.white_time > 0) {
        available_time = search_options.white_time - beast::infra::Constants::TIME_FLEX;
        time_for_move = std::min(
            static_cast<int>(0.1 * static_cast<double>(available_time))
                + search_options.white_increment,
            available_time
        );
    } else if (!white_to_move && search_options.black_time > 0 && search_options.black_increment == 0) {
        available_time = search_options.black_time - beast::infra::Constants::TIME_FLEX;
        time_for_move = static_cast<int>(0.05 * static_cast<double>(available_time));
    } else if (!white_to_move && search_options.black_time > 0) {
        available_time = search_options.black_time - beast::infra::Constants::TIME_FLEX;
        time_for_move = std::min(
            static_cast<int>(0.1 * static_cast<double>(available_time))
                + search_options.black_increment,
            available_time
        );
    } else {
        return;
    }

    available_time = std::max(1, available_time);
    time_for_move = std::max(1, std::min(time_for_move, available_time));
    deadline_ = std::chrono::steady_clock::now() + std::chrono::milliseconds(time_for_move);
}

void Engine::clearTimer() {
    deadline_.reset();
}

void Engine::search(chess::Board board, const int max_depth) {
    chess::Movelist root_moves;
    chess::movegen::legalmoves(root_moves, board);
    if (root_moves.empty()) {
        std::cout << "bestmove 0000" << std::endl;
        return;
    }

    std::vector<chess::Move> initial_moves(root_moves.begin(), root_moves.end());
    std::uniform_int_distribution<std::size_t> distribution(0, initial_moves.size() - 1);
    std::vector<chess::Move> best_line = {initial_moves[distribution(rng_)]};

    int depth = 0;
    const auto search_started = std::chrono::steady_clock::now();
    nodes_searched_ = 0;
    killers_.fill({chess::Move::NO_MOVE, chess::Move::NO_MOVE});
    history_.fill({});

    while (depth < max_depth) {
        ++depth;
        try {
            selective_depth_ = 0;
            auto [evaluation, moves] = negamax(
                board,
                depth,
                NEG_INF,
                POS_INF,
                0
            );
            if (!moves.empty()) {
                best_line = std::move(moves);
            }

            const auto current_time = std::chrono::steady_clock::now() - search_started;
            const auto elapsed_ms = std::max(
                1LL,
                std::chrono::duration_cast<std::chrono::milliseconds>(current_time).count()
            );
            const auto nps = static_cast<long long>(
                (1000.0 * static_cast<double>(nodes_searched_)) / static_cast<double>(elapsed_ms)
            );

            const double safe_eval = std::isfinite(evaluation) ? std::clamp(evaluation, NEG_INF, POS_INF) : 0.0;
            std::cout << "info depth " << depth << " seldepth " << selective_depth_ << " score cp "
                      << static_cast<int>(safe_eval) << " nodes " << nodes_searched_ << " nps " << nps
                      << " time " << elapsed_ms << " pv";
            for (const auto& move : best_line) {
                std::cout << " " << chess::uci::moveToUci(move);
            }
            std::cout << std::endl;
        } catch (const std::runtime_error&) {
            break;
        }
    }

    std::cout << "bestmove " << chess::uci::moveToUci(best_line.front()) << std::endl;
}

std::pair<double, std::vector<chess::Move>>
Engine::negamax(chess::Board& board, const int depth, double alpha, const double beta, const int ply) {
    selective_depth_ = std::max(selective_depth_, ply);
    ++nodes_searched_;
    checkStop();

    if (!std::isfinite(alpha)) {
        alpha = NEG_INF;
    }
    if (!std::isfinite(beta)) {
        return {0.0, {}};
    }
    if (alpha >= beta) {
        return {alpha, {}};
    }

    const double alpha_orig = alpha;
    const std::uint64_t key = board.hash();
    TTEntry* tt_entry = &tt_[key & (TT_SIZE - 1)];
    chess::Move tt_move = chess::Move::NO_MOVE;
    if (tt_entry->key == key && std::isfinite(tt_entry->score)) {
        if (tt_entry->best_move != chess::Move::NO_MOVE) {
            tt_move = chess::Move(tt_entry->best_move);
        }
        if (tt_entry->depth >= depth) {
            if (tt_entry->flag == TT_EXACT) {
                return {tt_entry->score, {}};
            }
            if (tt_entry->flag == TT_LOWER && tt_entry->score >= beta) {
                return {tt_entry->score, {}};
            }
            if (tt_entry->flag == TT_UPPER && tt_entry->score <= alpha) {
                return {tt_entry->score, {}};
            }
        }
    }

    const auto [game_reason, _game_result] = board.isGameOver();
    if (game_reason != chess::GameResultReason::NONE) {
        return {heuristic_->evaluateResult(board, depth), {}};
    }
    if (board.isHalfMoveDraw() || (board.halfMoveClock() >= 4 && board.isRepetition())) {
        return {0.0, {}};
    }
    if (depth == 0) {
        if (heuristic_->useQuiescence()) {
            return {quiescence(board, alpha, beta, ply), {}};
        }
        return {heuristic_->evaluatePosition(board, false), {}};
    }

    if (depth >= NULL_MOVE_MIN_DEPTH && ply > 0 && !board.inCheck() && beta < POS_INF - 1.0
        && static_cast<int>(board.occ().count()) >= NULL_MOVE_MIN_PIECES) {
        board.makeNullMove();
        auto [null_score, _null_line] = negamax(
            board,
            depth - 1 - NULL_MOVE_REDUCTION,
            -beta,
            -beta + 1.0,
            ply + 1
        );
        board.unmakeNullMove();

        const double null_eval = -null_score;
        if (null_eval >= beta) {
            *tt_entry = TTEntry{key, depth, null_eval, TT_LOWER, chess::Move::NO_MOVE};
            return {null_eval, {}};
        }
    }

    std::vector<chess::Move> best_moves;
    chess::Movelist moves;
    chess::movegen::legalmoves(moves, board);
    std::vector<chess::Move> sorted_moves(moves.begin(), moves.end());
    if (sorted_moves.empty()) {
        return {heuristic_->evaluatePosition(board, true), {}};
    }
    sorted_moves = orderMoves(board, sorted_moves, tt_move, ply);

    int move_index = 0;
    for (const auto& move : sorted_moves) {
        const bool is_capture = board.isCapture(move);
        const bool is_promotion = move.typeOf() == chess::Move::PROMOTION;
        const bool is_tactical = is_capture || is_promotion;
        board.makeMove(move);
        double evaluation = 0.0;
        std::vector<chess::Move> line;

        if (move_index == 0) {
            auto [child_score, child_line] = negamax(board, depth - 1, -beta, -alpha, ply + 1);
            evaluation = -child_score;
            line = std::move(child_line);
        } else {
            if (alpha <= NEG_INF + 1.0) {
                auto [full_score, full_line] = negamax(board, depth - 1, -beta, -alpha, ply + 1);
                evaluation = -full_score;
                line = std::move(full_line);
                board.unmakeMove(move);
                line.insert(line.begin(), move);

                if (evaluation >= beta) {
                    if (!is_tactical && ply < MAX_PLY) {
                        auto& killer = killers_[ply];
                        if (killer[0] != move) {
                            killer[1] = killer[0];
                            killer[0] = move;
                        }

                        const int side_index = board.sideToMove() == chess::Color::WHITE ? 0 : 1;
                        const int from = move.from().index();
                        const int to = move.to().index();
                        history_[side_index][from][to] = std::min(
                            history_[side_index][from][to] + depth * depth,
                            1 << 20
                        );
                    }

                    *tt_entry = TTEntry{
                        key,
                        depth,
                        std::isfinite(evaluation) ? evaluation : 0.0,
                        TT_LOWER,
                        static_cast<std::uint16_t>(move.move()),
                    };
                    return {evaluation, std::move(line)};
                }
                if (evaluation > alpha) {
                    alpha = evaluation;
                    best_moves = std::move(line);
                }
                ++move_index;
                continue;
            }

            const bool can_lmr = depth >= LMR_MIN_DEPTH && move_index >= LMR_MIN_MOVE_INDEX && !is_tactical
                && !board.inCheck() && ply > 0;
            int search_depth = depth - 1;
            if (can_lmr) {
                search_depth = std::max(0, search_depth - 1);
            }

            auto [child_score, child_line] = negamax(board, search_depth, -alpha - 1.0, -alpha, ply + 1);
            evaluation = -child_score;
            line = std::move(child_line);

            if (can_lmr && evaluation > alpha) {
                auto [verify_score, verify_line] = negamax(board, depth - 1, -alpha - 1.0, -alpha, ply + 1);
                evaluation = -verify_score;
                line = std::move(verify_line);
            }

            if (evaluation > alpha && evaluation < beta) {
                auto [full_score, full_line] = negamax(board, depth - 1, -beta, -alpha, ply + 1);
                evaluation = -full_score;
                line = std::move(full_line);
            }
        }

        board.unmakeMove(move);
        if (!std::isfinite(evaluation)) {
            evaluation = NEG_INF;
        }
        line.insert(line.begin(), move);

        if (evaluation >= beta) {
            if (!is_tactical && ply < MAX_PLY) {
                auto& killer = killers_[ply];
                if (killer[0] != move) {
                    killer[1] = killer[0];
                    killer[0] = move;
                }

                const int side_index = board.sideToMove() == chess::Color::WHITE ? 0 : 1;
                const int from = move.from().index();
                const int to = move.to().index();
                history_[side_index][from][to] = std::min(
                    history_[side_index][from][to] + depth * depth,
                    1 << 20
                );
            }

            *tt_entry = TTEntry{
                key,
                depth,
                std::isfinite(evaluation) ? evaluation : 0.0,
                TT_LOWER,
                static_cast<std::uint16_t>(move.move()),
            };
            return {evaluation, std::move(line)};
        }
        if (evaluation > alpha) {
            alpha = evaluation;
            best_moves = std::move(line);
        }
        ++move_index;
    }

    std::uint16_t best_move = chess::Move::NO_MOVE;
    if (!best_moves.empty()) {
        best_move = static_cast<std::uint16_t>(best_moves.front().move());
    }
    std::uint8_t bound = TT_EXACT;
    if (alpha <= alpha_orig) {
        bound = TT_UPPER;
    } else if (alpha >= beta) {
        bound = TT_LOWER;
    }
    *tt_entry = TTEntry{key, depth, std::isfinite(alpha) ? alpha : 0.0, bound, best_move};

    return {alpha, best_moves};
}

double Engine::quiescence(chess::Board& board, double alpha, const double beta, const int ply) {
    selective_depth_ = std::max(selective_depth_, ply);
    checkStop();

    const auto [game_reason, _game_result] = board.isGameOver();
    if (game_reason != chess::GameResultReason::NONE) {
        return heuristic_->evaluateResult(board, -1);
    }
    if (board.isHalfMoveDraw() || (board.halfMoveClock() >= 4 && board.isRepetition())) {
        return 0.0;
    }

    const double evaluation = heuristic_->evaluatePosition(board, false);
    if (evaluation >= beta) {
        return beta;
    }

    const bool use_delta_pruning = board.occ().count() > 8;
    if (use_delta_pruning && evaluation < alpha - static_cast<double>(beast::heuristics::PieceValues::QUEEN_VALUE)) {
        return alpha;
    }
    alpha = std::max(alpha, evaluation);

    for (const auto& move : getCaptures(board)) {
        if (use_delta_pruning) {
            const chess::PieceType captured_piece = (move.typeOf() == chess::Move::ENPASSANT)
                ? chess::PieceType::PAWN
                : board.at(move.to()).type();
            const int piece_value = beast::heuristics::PieceValues::value(captured_piece) + 200;
            if (evaluation + static_cast<double>(piece_value) < alpha) {
                continue;
            }
        }

        board.makeMove(move);
        const double score = -quiescence(board, -beta, -alpha, ply + 1);
        board.unmakeMove(move);
        ++nodes_searched_;

        if (score >= beta) {
            return beta;
        }
        alpha = std::max(alpha, score);
    }

    return alpha;
}

std::vector<chess::Move> Engine::getCaptures(const chess::Board& board) const {
    chess::Movelist moves;
    chess::movegen::legalmoves<chess::movegen::MoveGenType::CAPTURE>(moves, board);

    std::vector<chess::Move> captures(moves.begin(), moves.end());

    return orderMoves(board, captures, chess::Move::NO_MOVE, 0);
}

std::vector<chess::Move> Engine::orderMoves(
    const chess::Board& board,
    const std::vector<chess::Move>& moves,
    const chess::Move tt_move,
    const int ply
) const {
    std::vector<std::pair<chess::Move, int>> scored;
    scored.reserve(moves.size());
    const int side_index = board.sideToMove() == chess::Color::WHITE ? 0 : 1;
    const auto killer_slot = killers_[std::min(ply, MAX_PLY - 1)];

    for (const auto& move : moves) {
        int score = 0;
        if (tt_move.move() != chess::Move::NO_MOVE && move == tt_move) {
            score += 10'000'000;
        }
        if (board.isCapture(move)) {
            const auto victim_piece = board.at(move.to());
            const auto attacker_piece = board.at(move.from());
            if (victim_piece != chess::Piece::NONE && attacker_piece != chess::Piece::NONE) {
                score += 5'000'000 + beast::heuristics::PieceValues::value(victim_piece.type()) * 10
                    - beast::heuristics::PieceValues::value(attacker_piece.type());
            }
        } else {
            if (killer_slot[0].move() != chess::Move::NO_MOVE && move == killer_slot[0]) {
                score += 4'000'000;
            } else if (killer_slot[1].move() != chess::Move::NO_MOVE && move == killer_slot[1]) {
                score += 3'500'000;
            }

            score += history_[side_index][move.from().index()][move.to().index()];
        }
        if (move.typeOf() == chess::Move::PROMOTION) {
            score += beast::heuristics::PieceValues::value(move.promotionType()) * 5;
        }
        scored.emplace_back(move, score);
    }

    std::sort(
        scored.begin(),
        scored.end(),
        [](const auto& lhs, const auto& rhs) { return lhs.second > rhs.second; }
    );

    std::vector<chess::Move> sorted;
    sorted.reserve(scored.size());
    for (const auto& [move, _score] : scored) {
        sorted.push_back(move);
    }
    return sorted;
}

}  // namespace beast::engine
