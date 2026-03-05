#pragma once

#include "beast/heuristics/infra/heuristic.hpp"

namespace beast::heuristics {

class ClassicalHeuristic final : public Heuristic {
   public:
    ClassicalHeuristic(bool fifty_moves_rule, std::filesystem::path syzygy_path, int syzygy_probe_limit);

    [[nodiscard]] bool useQuiescence() const override;

   protected:
    [[nodiscard]] double evaluateInternal(const chess::Board& board) const override;

   private:
    static constexpr int PAWN_RANK_WEIGHT = 7;
    static constexpr int PAWN_FILE_WEIGHT = 5;
    static constexpr int PAWN_CENTER_WEIGHT = 5;
    static constexpr int PAWN_DISTANCE_WEIGHT = 5;

    static constexpr int KNIGHT_CENTER_WEIGHT = 7;
    static constexpr int KNIGHT_DISTANCE_WEIGHT = 8;

    static constexpr int BISHOP_CENTER_WEIGHT = 5;
    static constexpr int BISHOP_DISTANCE_WEIGHT = 8;

    static constexpr int ROOK_CENTER_WEIGHT = 8;
    static constexpr int ROOK_DISTANCE_WEIGHT = 5;

    static constexpr int QUEEN_CENTER_WEIGHT = 2;
    static constexpr int QUEEN_DISTANCE_WEIGHT = 8;

    static constexpr int KING_CENTER_WEIGHT = 8;
    static constexpr int KING_DISTANCE_WEIGHT = 5;

    [[nodiscard]] int pawnBonus(chess::Bitboard pawns, chess::Square king_position, bool color) const;
    [[nodiscard]] int knightBonus(chess::Bitboard knights, chess::Square king_position) const;
    [[nodiscard]] int bishopBonus(chess::Bitboard bishops, chess::Square king_position) const;
    [[nodiscard]] int rookBonus(chess::Bitboard rooks, chess::Square king_position) const;
    [[nodiscard]] int queenBonus(chess::Bitboard queens, chess::Square king_position) const;
    [[nodiscard]] int kingBonus(
        chess::Square king_position, chess::Square opponents_king, bool queens_on_board
    ) const;

    static int occupyingCenterBonus(int piece_position, int bonus);
    static int distanceFromKingBonus(int piece_position, int king_position, int bonus);
};

}  // namespace beast::heuristics

