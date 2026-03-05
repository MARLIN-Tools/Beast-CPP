#pragma once

#include <array>

#include "chess.hpp"

namespace beast::heuristics {

struct PieceValues {
    static constexpr int PAWN_VALUE = 100;
    static constexpr int KNIGHT_VALUE = 350;
    static constexpr int BISHOP_VALUE = 370;
    static constexpr int ROOK_VALUE = 550;
    static constexpr int QUEEN_VALUE = 950;

    static constexpr int value(chess::PieceType piece_type) {
        switch (piece_type.internal()) {
            case chess::PieceType::PAWN:
                return PAWN_VALUE;
            case chess::PieceType::KNIGHT:
                return KNIGHT_VALUE;
            case chess::PieceType::BISHOP:
                return BISHOP_VALUE;
            case chess::PieceType::ROOK:
                return ROOK_VALUE;
            case chess::PieceType::QUEEN:
                return QUEEN_VALUE;
            default:
                return 0;
        }
    }
};

}  // namespace beast::heuristics

