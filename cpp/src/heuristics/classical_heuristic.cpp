#include "beast/heuristics/classical_heuristic.hpp"

#include <cmath>
#include <utility>

#include "beast/heuristics/infra/piece_values.hpp"

namespace beast::heuristics {

ClassicalHeuristic::ClassicalHeuristic(
    const bool fifty_moves_rule, std::filesystem::path syzygy_path, const int syzygy_probe_limit
)
    : Heuristic(fifty_moves_rule, std::move(syzygy_path), syzygy_probe_limit) {}

bool ClassicalHeuristic::useQuiescence() const {
    return true;
}

double ClassicalHeuristic::evaluateInternal(const chess::Board& board) const {
    const auto w_pawns = board.pieces(chess::PieceType::PAWN, chess::Color::WHITE);
    const auto b_pawns = board.pieces(chess::PieceType::PAWN, chess::Color::BLACK);
    const auto w_knights = board.pieces(chess::PieceType::KNIGHT, chess::Color::WHITE);
    const auto b_knights = board.pieces(chess::PieceType::KNIGHT, chess::Color::BLACK);
    const auto w_bishops = board.pieces(chess::PieceType::BISHOP, chess::Color::WHITE);
    const auto b_bishops = board.pieces(chess::PieceType::BISHOP, chess::Color::BLACK);
    const auto w_rooks = board.pieces(chess::PieceType::ROOK, chess::Color::WHITE);
    const auto b_rooks = board.pieces(chess::PieceType::ROOK, chess::Color::BLACK);
    const auto w_queens = board.pieces(chess::PieceType::QUEEN, chess::Color::WHITE);
    const auto b_queens = board.pieces(chess::PieceType::QUEEN, chess::Color::BLACK);
    const auto w_king = board.kingSq(chess::Color::WHITE);
    const auto b_king = board.kingSq(chess::Color::BLACK);

    double evaluation = static_cast<double>(
        w_pawns.count() * PieceValues::PAWN_VALUE - b_pawns.count() * PieceValues::PAWN_VALUE
        + w_knights.count() * PieceValues::KNIGHT_VALUE - b_knights.count() * PieceValues::KNIGHT_VALUE
        + w_bishops.count() * PieceValues::BISHOP_VALUE - b_bishops.count() * PieceValues::BISHOP_VALUE
        + w_rooks.count() * PieceValues::ROOK_VALUE - b_rooks.count() * PieceValues::ROOK_VALUE
        + w_queens.count() * PieceValues::QUEEN_VALUE - b_queens.count() * PieceValues::QUEEN_VALUE
    );

    const auto wp_bonus = pawnBonus(w_pawns, b_king, true);
    const auto bp_bonus = pawnBonus(b_pawns, w_king, false);
    const auto wk_bonus = knightBonus(w_knights, b_king);
    const auto bk_bonus = knightBonus(b_knights, w_king);
    const auto wb_bonus = bishopBonus(w_bishops, b_king);
    const auto bb_bonus = bishopBonus(b_bishops, w_king);
    const auto wr_bonus = rookBonus(w_rooks, b_king);
    const auto br_bonus = rookBonus(b_rooks, w_king);
    const auto wq_bonus = queenBonus(w_queens, b_king);
    const auto bq_bonus = queenBonus(b_queens, w_king);
    const auto wki_bonus = kingBonus(w_king, b_king, b_queens.count() > 0);
    const auto bki_bonus = kingBonus(b_king, w_king, w_queens.count() > 0);

    evaluation += static_cast<double>(
        wp_bonus - bp_bonus + wk_bonus - bk_bonus + wb_bonus - bb_bonus + wr_bonus - br_bonus + wq_bonus
        - bq_bonus + wki_bonus - bki_bonus
    );

    if (board.sideToMove() == chess::Color::BLACK) {
        return static_cast<int>(-evaluation);
    }
    return static_cast<int>(evaluation);
}

int ClassicalHeuristic::pawnBonus(const chess::Bitboard pawns, const chess::Square king_position, const bool color)
    const {
    int bonus = 0;
    auto pieces = pawns;
    while (pieces) {
        const int pawn_position = pieces.pop();
        if (color) {
            bonus += (pawn_position / 8 - 1) * PAWN_RANK_WEIGHT;
        } else {
            bonus += (6 - pawn_position / 8) * PAWN_RANK_WEIGHT;
        }

        if (pawn_position % 8 < 3) {
            bonus -= (3 - pawn_position % 8) * PAWN_FILE_WEIGHT;
        } else if (pawn_position % 8 > 4) {
            bonus -= (pawn_position % 8 - 4) * PAWN_FILE_WEIGHT;
        }

        bonus += occupyingCenterBonus(pawn_position, PAWN_CENTER_WEIGHT);
        bonus += distanceFromKingBonus(pawn_position, king_position.index(), PAWN_DISTANCE_WEIGHT);
    }
    return bonus;
}

int ClassicalHeuristic::knightBonus(const chess::Bitboard knights, const chess::Square king_position) const {
    int bonus = 0;
    auto pieces = knights;
    while (pieces) {
        const int piece_position = pieces.pop();
        bonus += occupyingCenterBonus(piece_position, KNIGHT_CENTER_WEIGHT);
        bonus += distanceFromKingBonus(piece_position, king_position.index(), KNIGHT_DISTANCE_WEIGHT);
    }
    return bonus;
}

int ClassicalHeuristic::bishopBonus(const chess::Bitboard bishops, const chess::Square king_position) const {
    int bonus = 0;
    auto pieces = bishops;
    while (pieces) {
        const int piece_position = pieces.pop();
        bonus += occupyingCenterBonus(piece_position, BISHOP_CENTER_WEIGHT);
        bonus += distanceFromKingBonus(piece_position, king_position.index(), BISHOP_DISTANCE_WEIGHT);
    }
    return bonus;
}

int ClassicalHeuristic::rookBonus(const chess::Bitboard rooks, const chess::Square king_position) const {
    int bonus = 0;
    auto pieces = rooks;
    while (pieces) {
        const int piece_position = pieces.pop();
        if (piece_position % 8 >= 3 && piece_position % 8 < 5) {
            bonus += ROOK_CENTER_WEIGHT;
        }
        if (piece_position % 8 >= 2 && piece_position % 8 < 6) {
            bonus += ROOK_CENTER_WEIGHT;
        }
        if (piece_position % 8 >= 1 && piece_position % 8 < 7) {
            bonus += ROOK_CENTER_WEIGHT;
        }
        bonus += distanceFromKingBonus(piece_position, king_position.index(), ROOK_DISTANCE_WEIGHT);
    }
    return bonus;
}

int ClassicalHeuristic::queenBonus(const chess::Bitboard queens, const chess::Square king_position) const {
    int bonus = 0;
    auto pieces = queens;
    while (pieces) {
        const int piece_position = pieces.pop();
        bonus += occupyingCenterBonus(piece_position, QUEEN_CENTER_WEIGHT);
        bonus += distanceFromKingBonus(piece_position, king_position.index(), QUEEN_DISTANCE_WEIGHT);
    }
    return bonus;
}

int ClassicalHeuristic::kingBonus(
    const chess::Square king_position, const chess::Square opponents_king, const bool queens_on_board
) const {
    int bonus = 0;
    const int king_center_weight = queens_on_board ? -KING_CENTER_WEIGHT : KING_CENTER_WEIGHT;

    bonus += occupyingCenterBonus(king_position.index(), king_center_weight);
    bonus += distanceFromKingBonus(king_position.index(), opponents_king.index(), KING_DISTANCE_WEIGHT);
    return bonus;
}

int ClassicalHeuristic::occupyingCenterBonus(const int piece_position, const int bonus) {
    const int rank = piece_position / 8;
    const int file = piece_position % 8;

    if (rank >= 3 && rank < 5 && file >= 3 && file < 5) {
        return 3 * bonus;
    }
    if (rank >= 2 && rank < 6 && file >= 2 && file < 6) {
        return 2 * bonus;
    }
    if (rank >= 1 && rank < 7 && file >= 1 && file < 7) {
        return bonus;
    }
    return 0;
}

int ClassicalHeuristic::distanceFromKingBonus(
    const int piece_position, const int king_position, const int bonus
) {
    const int distance = std::abs(piece_position / 8 - king_position / 8)
        + std::abs(piece_position % 8 - king_position % 8);
    return static_cast<int>(14.0 / static_cast<double>(distance) * bonus - bonus);
}

}  // namespace beast::heuristics
