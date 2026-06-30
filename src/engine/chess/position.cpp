#include "position.hpp"

#include <sstream>

#include "zobrist.hpp"

namespace episteme {
Position::Position() : history{}, current{} { history.reserve(1024); }

void Position::from_FEN(const std::string& FEN) {
    history.clear();
    history.shrink_to_fit();

    std::array<std::string, 6> tokens;
    size_t i = 0;

    std::istringstream iss(FEN);
    std::string token;
    while (iss >> token && i < tokens.size()) {
        tokens[i++] = token;
    }

    current.mailbox.fill(Piece::None);
    current.bitboards.fill(0);

    size_t square_idx = 56;
    for (char c : tokens[0]) {
        if (c == '/') {
            square_idx -= 16;
        } else if (std::isdigit(c)) {
            square_idx += c - '0';
        } else {
            auto it = piece_map.find(c);
            if (it != piece_map.end()) {
                PieceType type = it->second.first;
                Color color = it->second.second;
                Piece piece = piece_type_with_color(type, color);
                uint64_t sq = (uint64_t)1 << square_idx;

                current.bitboards[piece_type_idx(type)] ^= sq;
                current.bitboards[color_idx(color) + COLOR_OFFSET] ^= sq;
                current.mailbox[square_idx] = piece;
            }
            ++square_idx;
        }
    }

    current.stm = (tokens[1] == "w") ? 0 : 1;

    if (tokens[2] != "-") {
        for (char c : tokens[2]) {
            switch (c) {
                case 'K':
                    current.allowed_castles.rooks[color_idx(Color::White)].kingside = Square::H1;
                    break;
                case 'Q':
                    current.allowed_castles.rooks[color_idx(Color::White)].queenside = Square::A1;
                    break;
                case 'k':
                    current.allowed_castles.rooks[color_idx(Color::Black)].kingside = Square::H8;
                    break;
                case 'q':
                    current.allowed_castles.rooks[color_idx(Color::Black)].queenside = Square::A8;
                    break;
            }
        }
    }

    if (tokens[3] != "-") {
        current.ep_square = static_cast<Square>((tokens[3][0] - 'a') + (tokens[3][1] - '1') * 8);
    }

    current.half_move_clock = std::stoi(tokens[4]);
    current.full_move_number = std::stoi(tokens[5]);

    current.hashes = explicit_hashes();

    history.push_back(current);
}

void Position::from_startpos() {
    from_FEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
}

void Position::make_move(const Move& move) {
    const Square sq_src = move.from_square();
    const Square sq_dst = move.to_square();

    const Piece mover = current.mailbox[sq_idx(sq_src)];
    const Piece captured = current.mailbox[sq_idx(sq_dst)];

    const Color side = STM();
    const Color xside = flip(side);
    const auto us = color_idx(side);

    auto toggle_hashes = [&](Piece piece, Square square) {
        const uint64_t h = hash::piecesquares[piecesquare(piece, square, false)];
        Hashes& hashes = current.hashes;
        hashes.full_hash ^= h;

        const PieceType type = piece_type(piece);
        if (type == PieceType::Pawn) {
            hashes.pawn_hash ^= h;
        } else if (color(piece) == Color::Black) {
            hashes.non_pawn_black_hash ^= h;
        } else {
            hashes.non_pawn_white_hash ^= h;
        }

        if (type == PieceType::Rook || type == PieceType::Queen || type == PieceType::King)
            hashes.major_hash ^= h;
        if (type == PieceType::Knight || type == PieceType::Bishop || type == PieceType::King)
            hashes.minor_hash ^= h;
    };

    auto toggle_bitboards = [&](Piece piece, Square square) {
        const uint64_t bb = (uint64_t)1 << sq_idx(square);
        current.bitboards[piece_type_idx(piece_type(piece))] ^= bb;
        current.bitboards[color_idx(piece) + COLOR_OFFSET] ^= bb;
    };

    auto add_piece = [&](Piece piece, Square square) {
        toggle_bitboards(piece, square);
        toggle_hashes(piece, square);
        current.mailbox[sq_idx(square)] = piece;
    };

    auto remove_piece = [&](Piece piece, Square square) {
        toggle_bitboards(piece, square);
        toggle_hashes(piece, square);
        current.mailbox[sq_idx(square)] = Piece::None;
    };

    auto move_piece = [&](Piece piece, Square from, Square to) {
        remove_piece(piece, from);
        add_piece(piece, to);
    };

    auto edit_castling = [&](auto&& edit) {
        current.hashes.full_hash ^= hash::castling_rights[current.allowed_castles.as_mask()];
        edit();
        current.hashes.full_hash ^= hash::castling_rights[current.allowed_castles.as_mask()];
    };

    auto revoke_rook_right = [&](Color for_side, Square square) {
        auto& rooks = current.allowed_castles.rooks[color_idx(for_side)];
        if (square == rooks.kingside)
            rooks.unset(true);
        else if (square == rooks.queenside)
            rooks.unset(false);
    };

    auto capture_on = [&](Piece victim, Square square) {
        remove_piece(victim, square);
        if (piece_type(victim) == PieceType::Rook)
            edit_castling([&] { revoke_rook_right(xside, square); });
    };

    if (current.ep_square != Square::None) {
        current.hashes.full_hash ^= hash::ep_files[file(current.ep_square)];
        current.ep_square = Square::None;
    }

    if (piece_type(mover) == PieceType::Pawn || captured != Piece::None) {
        current.half_move_clock = 0;
    } else {
        current.half_move_clock++;
    }

    if (side == Color::Black) {
        current.full_move_number++;
    }

    switch (move.move_type()) {
        case MoveType::Normal: {
            if (captured != Piece::None) capture_on(captured, sq_dst);

            if (piece_type(mover) == PieceType::King) {
                edit_castling([&] { current.allowed_castles.rooks[us].clear(); });
            } else if (piece_type(mover) == PieceType::Rook) {
                edit_castling([&] { revoke_rook_right(side, sq_src); });
            }

            if (piece_type(mover) == PieceType::Pawn &&
                std::abs(sq_idx(sq_src) - sq_idx(sq_dst)) == DOUBLE_PUSH) {
                current.ep_square = sq_from_idx(sq_idx(sq_dst) + ((side == Color::White) ? -8 : 8));
                current.hashes.full_hash ^= hash::ep_files[file(sq_dst)];
            }

            move_piece(mover, sq_src, sq_dst);
            break;
        }

        case MoveType::Castling: {
            const bool king_side = sq_idx(sq_dst) > sq_idx(sq_src);
            const Square rook_src = king_side ? current.allowed_castles.rooks[us].kingside
                                              : current.allowed_castles.rooks[us].queenside;
            const Square rook_dst = sq_from_idx(sq_idx(sq_dst) + (king_side ? -1 : 1));

            move_piece(mover, sq_src, sq_dst);
            move_piece(piece_type_with_color(PieceType::Rook, side), rook_src, rook_dst);

            edit_castling([&] { current.allowed_castles.rooks[us].clear(); });
            break;
        }

        case MoveType::EnPassant: {
            const Square capture_sq =
                sq_from_idx(sq_idx(sq_dst) + ((side == Color::White) ? -8 : 8));

            move_piece(mover, sq_src, sq_dst);
            remove_piece(piece_type_with_color(PieceType::Pawn, xside), capture_sq);
            break;
        }

        case MoveType::Promotion: {
            if (captured != Piece::None) capture_on(captured, sq_dst);

            remove_piece(mover, sq_src);
            add_piece(piece_type_with_color(move.promo_piece_type(), side), sq_dst);
            break;
        }

        case MoveType::None:
            break;
    }

    current.stm = !current.stm;
    current.hashes.full_hash ^= hash::stm;

    history.push_back(current);
}

void Position::make_null() {
    if (current.ep_square != Square::None) {
        current.hashes.full_hash ^= hash::ep_files[file(current.ep_square)];
        current.ep_square = Square::None;
    }

    current.half_move_clock++;

    if (STM() == Color::Black) {
        current.full_move_number++;
    }

    current.stm = !current.stm;
    current.hashes.full_hash ^= hash::stm;

    history.push_back(current);
}

void Position::unmake_move() {
    history.pop_back();
    const PositionState& prev = history.back();
    current = prev;
}

bool Position::is_repetition(int rep_count) {
    int reps = 0;
    for (const PositionState& prev_state : history) {
        if (prev_state.hashes.full_hash == current.hashes.full_hash) {
            if (++reps == rep_count) return true;
        }
    }
    return false;
}

bool Position::is_insufficient() {
    if (current.bitboards[piece_type_idx(PieceType::Pawn)]) return false;
    if (current.bitboards[piece_type_idx(PieceType::Queen)] |
        current.bitboards[piece_type_idx(PieceType::Rook)])
        return false;
    if ((current.bitboards[piece_type_idx(PieceType::Bishop)] &
         current.bitboards[color_idx(Color::White) + COLOR_OFFSET]) &&
        (current.bitboards[piece_type_idx(PieceType::Bishop)] &
         current.bitboards[color_idx(Color::Black) + COLOR_OFFSET]))
        return false;
    if (current.bitboards[piece_type_idx(PieceType::Bishop)] &&
        current.bitboards[piece_type_idx(PieceType::Knight)])
        return false;
    if (std::popcount(current.bitboards[piece_type_idx(PieceType::Knight)])) return false;
    return true;
}

std::string Position::to_FEN() const {
    std::string fen;

    for (int rank = 7; rank >= 0; --rank) {
        int empty = 0;
        for (int file = 0; file < 8; ++file) {
            Piece piece = current.mailbox[rank * 8 + file];
            if (piece == Piece::None) {
                ++empty;
            } else {
                if (empty != 0) {
                    fen += std::to_string(empty);
                    empty = 0;
                }
                char c;
                PieceType pt = piece_type(piece);
                Color col = color(piece);
                switch (pt) {
                    case PieceType::Pawn:
                        c = 'p';
                        break;
                    case PieceType::Knight:
                        c = 'n';
                        break;
                    case PieceType::Bishop:
                        c = 'b';
                        break;
                    case PieceType::Rook:
                        c = 'r';
                        break;
                    case PieceType::Queen:
                        c = 'q';
                        break;
                    case PieceType::King:
                        c = 'k';
                        break;
                    default:
                        c = '?';
                }
                fen += (col == Color::White) ? std::toupper(c) : c;
            }
        }
        if (empty != 0) fen += std::to_string(empty);
        if (rank != 0) fen += '/';
    }

    fen += (current.stm == static_cast<bool>(Color::White)) ? " w " : " b ";

    std::string castling;
    if (current.allowed_castles.rooks[color_idx(Color::White)].is_kingside_set()) castling += 'K';
    if (current.allowed_castles.rooks[color_idx(Color::White)].is_queenside_set()) castling += 'Q';
    if (current.allowed_castles.rooks[color_idx(Color::Black)].is_kingside_set()) castling += 'k';
    if (current.allowed_castles.rooks[color_idx(Color::Black)].is_queenside_set()) castling += 'q';
    fen += (castling.empty() ? "-" : castling) + " ";

    fen += (current.ep_square == Square::None
                ? "-"
                : Move(current.ep_square, current.ep_square).to_string().substr(2)) +
           " ";

    fen += std::to_string(current.half_move_clock) + " " + std::to_string(current.full_move_number);

    return fen;
}

Hashes Position::explicit_hashes() {
    Hashes hashes{};

    for (size_t i = 0; i < 64; i++) {
        Piece piece = current.mailbox[i];
        if (piece != Piece::None) {
            uint64_t piece_hash = hash::piecesquares[piecesquare(piece, sq_from_idx(i), false)];
            PieceType type = piece_type(piece);

            hashes.full_hash ^= piece_hash;

            if (type == PieceType::Pawn) {
                hashes.pawn_hash ^= piece_hash;
            } else {
                if (color(piece) == Color::Black)
                    hashes.non_pawn_black_hash ^= piece_hash;
                else
                    hashes.non_pawn_white_hash ^= piece_hash;
            }

            if (type == PieceType::Rook || type == PieceType::Queen || type == PieceType::King) {
                hashes.major_hash ^= piece_hash;
            }
            if (type == PieceType::Knight || type == PieceType::Bishop || type == PieceType::King) {
                hashes.minor_hash ^= piece_hash;
            }
        }
    }

    if (!current.stm) hashes.full_hash ^= hash::stm;
    if (current.ep_square != Square::None)
        hashes.full_hash ^= hash::ep_files[file(current.ep_square)];
    hashes.full_hash ^= hash::castling_rights[current.allowed_castles.as_mask()];

    return hashes;
}

Move from_UCI(const Position& position, const std::string& move) {
    std::string src_str = move.substr(0, 2);
    std::string dst_str = move.substr(2, 2);

    auto str_2_sq = [](const std::string& square) {
        int file = square[0] - 'a';
        int rank = square[1] - '1';
        return static_cast<Square>(rank * 8 + file);
    };

    Square src = str_2_sq(src_str);
    Square dst = str_2_sq(dst_str);

    auto is_castling = [&]() {
        Color stm = position.STM();
        if (piece_type(position.mailbox(sq_idx(src))) != PieceType::King) return false;

        Square kingside = (stm == Color::White) ? Square::G1 : Square::G8;
        Square queenside = (stm == Color::White) ? Square::C1 : Square::C8;

        bool kingside_castle = (dst == kingside) && position.castling_rights(stm).is_kingside_set();
        bool queenside_castle =
            (dst == queenside) && position.castling_rights(stm).is_queenside_set();

        return (kingside_castle || queenside_castle);
    };

    bool is_promo = (move.length() == 5);
    bool is_en_passant = (piece_type(position.mailbox(sq_idx(src))) == PieceType::Pawn) &&
                         (dst == position.ep_square());

    auto char_2_piece = [&](char promo) {
        switch (promo) {
            case ('q'):
                return PromoPiece::Queen;
            case ('r'):
                return PromoPiece::Rook;
            case ('b'):
                return PromoPiece::Bishop;
            case ('n'):
                return PromoPiece::Knight;
            default:
                return PromoPiece::None;
        }
    };

    if (is_castling())
        return Move(src, dst, MoveType::Castling);
    else if (is_en_passant)
        return Move(src, dst, MoveType::EnPassant);
    else if (is_promo)
        return Move(src, dst, MoveType::Promotion, char_2_piece(move.at(4)));
    else
        return Move(src, dst);
}
}  // namespace episteme
