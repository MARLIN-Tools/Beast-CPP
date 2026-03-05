#include "beast/neural_networks/net_input_v2.hpp"

#include <algorithm>

namespace beast::neural_networks {

namespace {

constexpr int SQUARE_COUNT = 64;
constexpr int V2_PLANE_COUNT = 17;

int toModelSquareIndex(const int square_index) {
    // Preserve the old FEN-parser orientation (A8..H1 indexing).
    return square_index ^ 56;
}

void fillOneHotPlane(std::vector<float>& input, const int plane, const chess::Bitboard pieces) {
    auto remaining = pieces;
    while (remaining) {
        const int square_index = remaining.pop();
        input[plane * SQUARE_COUNT + toModelSquareIndex(square_index)] = 1.0F;
    }
}

}  // namespace

void boardToInputV2(const chess::Board& board, std::vector<float>& input) {
    const std::size_t target_size = static_cast<std::size_t>(V2_PLANE_COUNT * SQUARE_COUNT);
    if (input.size() != target_size) {
        input.assign(target_size, 0.0F);
    } else {
        std::fill(input.begin(), input.end(), 0.0F);
    }

    fillOneHotPlane(input, 0, board.pieces(chess::PieceType::PAWN, chess::Color::WHITE));
    fillOneHotPlane(input, 1, board.pieces(chess::PieceType::KNIGHT, chess::Color::WHITE));
    fillOneHotPlane(input, 2, board.pieces(chess::PieceType::BISHOP, chess::Color::WHITE));
    fillOneHotPlane(input, 3, board.pieces(chess::PieceType::ROOK, chess::Color::WHITE));
    fillOneHotPlane(input, 4, board.pieces(chess::PieceType::QUEEN, chess::Color::WHITE));
    fillOneHotPlane(input, 5, board.pieces(chess::PieceType::KING, chess::Color::WHITE));

    fillOneHotPlane(input, 6, board.pieces(chess::PieceType::PAWN, chess::Color::BLACK));
    fillOneHotPlane(input, 7, board.pieces(chess::PieceType::KNIGHT, chess::Color::BLACK));
    fillOneHotPlane(input, 8, board.pieces(chess::PieceType::BISHOP, chess::Color::BLACK));
    fillOneHotPlane(input, 9, board.pieces(chess::PieceType::ROOK, chess::Color::BLACK));
    fillOneHotPlane(input, 10, board.pieces(chess::PieceType::QUEEN, chess::Color::BLACK));
    fillOneHotPlane(input, 11, board.pieces(chess::PieceType::KING, chess::Color::BLACK));

    const float side_to_move = board.sideToMove() == chess::Color::WHITE ? 1.0F : 0.0F;
    for (int square = 0; square < SQUARE_COUNT; ++square) {
        input[12 * SQUARE_COUNT + square] = side_to_move;
    }

    const auto rights = board.castlingRights();
    const bool white_kingside =
        rights.has(chess::Color::WHITE, chess::Board::CastlingRights::Side::KING_SIDE);
    const bool white_queenside =
        rights.has(chess::Color::WHITE, chess::Board::CastlingRights::Side::QUEEN_SIDE);
    const bool black_kingside =
        rights.has(chess::Color::BLACK, chess::Board::CastlingRights::Side::KING_SIDE);
    const bool black_queenside =
        rights.has(chess::Color::BLACK, chess::Board::CastlingRights::Side::QUEEN_SIDE);

    for (int square = 0; square < SQUARE_COUNT; ++square) {
        input[13 * SQUARE_COUNT + square] = white_kingside ? 1.0F : 0.0F;
        input[14 * SQUARE_COUNT + square] = white_queenside ? 1.0F : 0.0F;
        input[15 * SQUARE_COUNT + square] = black_kingside ? 1.0F : 0.0F;
        input[16 * SQUARE_COUNT + square] = black_queenside ? 1.0F : 0.0F;
    }
}

}  // namespace beast::neural_networks
