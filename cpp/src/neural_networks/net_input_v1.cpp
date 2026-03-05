#include "beast/neural_networks/net_input_v1.hpp"

#include <algorithm>

namespace beast::neural_networks {

namespace {

constexpr int SQUARE_COUNT = 64;
constexpr int V1_PLANE_COUNT = 7;

int toModelSquareIndex(const int square_index) {
    // Preserve the old FEN-parser orientation (A8..H1 indexing).
    return square_index ^ 56;
}

void fillSignedPlane(
    std::vector<float>& input, const int plane, const chess::Bitboard pieces, const float value
) {
    auto remaining = pieces;
    while (remaining) {
        const int square_index = remaining.pop();
        input[plane * SQUARE_COUNT + toModelSquareIndex(square_index)] = value;
    }
}

}  // namespace

void boardToInputV1(const chess::Board& board, std::vector<float>& input) {
    const std::size_t target_size = static_cast<std::size_t>(V1_PLANE_COUNT * SQUARE_COUNT);
    if (input.size() != target_size) {
        input.assign(target_size, 0.0F);
    } else {
        std::fill(input.begin(), input.end(), 0.0F);
    }

    fillSignedPlane(input, 0, board.pieces(chess::PieceType::PAWN, chess::Color::WHITE), 1.0F);
    fillSignedPlane(input, 0, board.pieces(chess::PieceType::PAWN, chess::Color::BLACK), -1.0F);
    fillSignedPlane(input, 1, board.pieces(chess::PieceType::KNIGHT, chess::Color::WHITE), 1.0F);
    fillSignedPlane(input, 1, board.pieces(chess::PieceType::KNIGHT, chess::Color::BLACK), -1.0F);
    fillSignedPlane(input, 2, board.pieces(chess::PieceType::BISHOP, chess::Color::WHITE), 1.0F);
    fillSignedPlane(input, 2, board.pieces(chess::PieceType::BISHOP, chess::Color::BLACK), -1.0F);
    fillSignedPlane(input, 3, board.pieces(chess::PieceType::ROOK, chess::Color::WHITE), 1.0F);
    fillSignedPlane(input, 3, board.pieces(chess::PieceType::ROOK, chess::Color::BLACK), -1.0F);
    fillSignedPlane(input, 4, board.pieces(chess::PieceType::QUEEN, chess::Color::WHITE), 1.0F);
    fillSignedPlane(input, 4, board.pieces(chess::PieceType::QUEEN, chess::Color::BLACK), -1.0F);
    fillSignedPlane(input, 5, board.pieces(chess::PieceType::KING, chess::Color::WHITE), 1.0F);
    fillSignedPlane(input, 5, board.pieces(chess::PieceType::KING, chess::Color::BLACK), -1.0F);

    const float side_to_move_value = (board.sideToMove() == chess::Color::WHITE) ? 1.0F : -1.0F;
    for (int square = 0; square < SQUARE_COUNT; ++square) {
        input[6 * SQUARE_COUNT + square] = side_to_move_value;
    }
}

}  // namespace beast::neural_networks
