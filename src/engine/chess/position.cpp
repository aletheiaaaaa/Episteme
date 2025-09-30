#include "position.h"

namespace episteme {
    Position::Position() : history{}, current{} {
        history.reserve(1024);
    }

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
                    case 'K': current.allowed_castles.rooks[color_idx(Color::White)].kingside = Square::H1; break;
                    case 'Q': current.allowed_castles.rooks[color_idx(Color::White)].queenside = Square::A1; break;
                    case 'k': current.allowed_castles.rooks[color_idx(Color::Black)].kingside = Square::H8; break;
                    case 'q': current.allowed_castles.rooks[color_idx(Color::Black)].queenside = Square::A8; break;
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
        history.clear();
        history.shrink_to_fit();

        current.bitboards[piece_type_idx(PieceType::Pawn)]   = 0x00FF00000000FF00;
        current.bitboards[piece_type_idx(PieceType::Knight)] = 0x4200000000000042;
        current.bitboards[piece_type_idx(PieceType::Bishop)] = 0x2400000000000024;
        current.bitboards[piece_type_idx(PieceType::Rook)]   = 0x8100000000000081;
        current.bitboards[piece_type_idx(PieceType::Queen)]  = 0x0800000000000008;
        current.bitboards[piece_type_idx(PieceType::King)]   = 0x1000000000000010;

        current.bitboards[color_idx(Color::White) + COLOR_OFFSET] = 0x000000000000FFFF;
        current.bitboards[color_idx(Color::Black) + COLOR_OFFSET] = 0xFFFF000000000000;

        current.mailbox.fill(Piece::None);

        auto setup_rank = [&](int rank, Color color) {
            current.mailbox[sq_idx(static_cast<Square>(rank * 8 + 0))] = piece_type_with_color(PieceType::Rook, color);
            current.mailbox[sq_idx(static_cast<Square>(rank * 8 + 1))] = piece_type_with_color(PieceType::Knight, color);
            current.mailbox[sq_idx(static_cast<Square>(rank * 8 + 2))] = piece_type_with_color(PieceType::Bishop, color);
            current.mailbox[sq_idx(static_cast<Square>(rank * 8 + 3))] = piece_type_with_color(PieceType::Queen, color);
            current.mailbox[sq_idx(static_cast<Square>(rank * 8 + 4))] = piece_type_with_color(PieceType::King, color);
            current.mailbox[sq_idx(static_cast<Square>(rank * 8 + 5))] = piece_type_with_color(PieceType::Bishop, color);
            current.mailbox[sq_idx(static_cast<Square>(rank * 8 + 6))] = piece_type_with_color(PieceType::Knight, color);
            current.mailbox[sq_idx(static_cast<Square>(rank * 8 + 7))] = piece_type_with_color(PieceType::Rook, color);
        };

        setup_rank(0, Color::White);
        setup_rank(7, Color::Black);

        for (int file = 0; file < 8; ++file) {
            current.mailbox[sq_idx(static_cast<Square>(8 + file))] = Piece::WhitePawn;
            current.mailbox[sq_idx(static_cast<Square>(48 + file))] = Piece::BlackPawn;
        }

        current.allowed_castles.rooks[color_idx(Color::White)].kingside  = Square::H1;
        current.allowed_castles.rooks[color_idx(Color::White)].queenside = Square::A1;
        current.allowed_castles.rooks[color_idx(Color::Black)].kingside  = Square::H8;
        current.allowed_castles.rooks[color_idx(Color::Black)].queenside = Square::A8;

        current.stm = 0;
        current.half_move_clock = 0;
        current.full_move_number = 1;
        current.ep_square = Square::None;

        current.hashes = explicit_hashes();

        history.push_back(current);
    }

    void Position::make_move(const Move& move) {
        Square sq_src = move.from_square();
        Square sq_dst = move.to_square();

        Piece& src = current.mailbox[sq_idx(sq_src)];
        Piece& dst = current.mailbox[sq_idx(sq_dst)];

        uint64_t bb_src = (uint64_t)1 << sq_idx(sq_src);
        uint64_t bb_dst = (uint64_t)1 << sq_idx(sq_dst);

        Color side = STM();
        auto us = color_idx(side);
        auto them = color_idx(flip(side));

        if (current.ep_square != Square::None) {
            current.hashes.full_hash ^= hash::ep_files[file(current.ep_square)];
            current.ep_square = Square::None;
        } 

        if (piece_type(src) == PieceType::Pawn || dst != Piece::None) {
            current.half_move_clock = 0;
        } else {
            current.half_move_clock++;
        }

        if (side == Color::Black) {
            current.full_move_number++;
        }

        uint64_t attacker_src_hash = hash::piecesquares[piecesquare(src, sq_src, false)];
        uint64_t attacker_dst_hash = hash::piecesquares[piecesquare(src, sq_dst, false)];
        uint64_t victim_hash = (dst == Piece::None) ? 0 : hash::piecesquares[piecesquare(dst, sq_dst, false)];

        current.hashes.full_hash ^= attacker_src_hash;

        switch (move.move_type()) {
            case MoveType::Normal: {
                if (dst != Piece::None) {
                    current.hashes.full_hash ^= victim_hash;
                    current.bitboards[piece_type_idx(piece_type(dst))] ^= bb_dst;
                    current.bitboards[them + COLOR_OFFSET] ^= bb_dst;

                    if (piece_type(dst) == PieceType::Rook) {
                        current.hashes.full_hash ^= hash::castling_rights[current.allowed_castles.as_mask()];
                        auto& rooks = current.allowed_castles.rooks[them];
                        if (sq_dst == rooks.kingside) rooks.unset(true);
                        else if (sq_dst == rooks.queenside) rooks.unset(false);
                        current.hashes.full_hash ^= hash::castling_rights[current.allowed_castles.as_mask()];
                    }
                }

                if (piece_type(src) == PieceType::King) {
                    current.hashes.full_hash ^= hash::castling_rights[current.allowed_castles.as_mask()];
                    current.allowed_castles.rooks[us].clear();
                    current.hashes.full_hash ^= hash::castling_rights[current.allowed_castles.as_mask()];
                } else if (piece_type(src) == PieceType::Rook) {
                    current.hashes.full_hash ^= hash::castling_rights[current.allowed_castles.as_mask()];
                    auto& rooks = current.allowed_castles.rooks[us];
                    if (sq_src == rooks.kingside) rooks.unset(true);
                    else if (sq_src == rooks.queenside) rooks.unset(false);
                    current.hashes.full_hash ^= hash::castling_rights[current.allowed_castles.as_mask()];
                }

                if (piece_type(src) == PieceType::Pawn && std::abs(sq_idx(sq_src) - sq_idx(sq_dst)) == DOUBLE_PUSH) {
                    current.ep_square = sq_from_idx(sq_idx(sq_dst) + ((side == Color::White) ? -8 : 8));
                    current.hashes.full_hash ^= hash::ep_files[file(sq_dst)];
                }

                current.hashes.full_hash ^= attacker_dst_hash;
                current.bitboards[piece_type_idx(piece_type(src))] ^= bb_src ^ bb_dst;
                current.bitboards[us + COLOR_OFFSET] ^= bb_src ^ bb_dst;

                auto update_hash = [&](Piece piece, uint64_t hash) {
                    switch (piece_type(piece)) {
                        case PieceType::Pawn: current.hashes.pawn_hash ^= hash; break;
                        case PieceType::Knight: case PieceType::Bishop: current.hashes.minor_hash ^= hash; break;
                        case PieceType::Rook: case PieceType::Queen: current.hashes.major_hash ^= hash; break;
                        case PieceType::King: current.hashes.major_hash ^= hash; current.hashes.minor_hash ^= hash; break;
                        default: break;
                    }
                    if (piece_type(piece) != PieceType::Pawn) {
                        if (color(piece) == Color::Black) current.hashes.non_pawn_black_hash ^= hash;
                        else current.hashes.non_pawn_white_hash ^= hash;
                    }
                };

                update_hash(src, attacker_src_hash ^ attacker_dst_hash);
                if (dst != Piece::None) update_hash(dst, victim_hash);

                dst = src;
                break;
            }

            case MoveType::Castling: {
                bool king_side = bb_dst > bb_src;
                Square rook_src = king_side ? current.allowed_castles.rooks[us].kingside : current.allowed_castles.rooks[us].queenside;
                Square rook_dst = static_cast<Square>(sq_idx(sq_dst) + (king_side ? -1 : 1));
                uint64_t bb_rook_src = (uint64_t)1 << sq_idx(rook_src);
                uint64_t bb_rook_dst = (uint64_t)1 << sq_idx(rook_dst);

                Piece rook_piece = piece_type_with_color(PieceType::Rook, side);

                current.hashes.full_hash ^= 
                    hash::piecesquares[piecesquare(rook_piece, rook_src, false)] ^
                    hash::piecesquares[piecesquare(rook_piece, rook_dst, false)] ^
                    hash::piecesquares[piecesquare(src, sq_dst, false)];

                current.bitboards[piece_type_idx(PieceType::Rook)] ^= bb_rook_src ^ bb_rook_dst;
                current.bitboards[piece_type_idx(PieceType::King)] ^= bb_src ^ bb_dst;
                current.bitboards[us + COLOR_OFFSET] ^= bb_rook_src ^ bb_rook_dst ^ bb_src ^ bb_dst;

                current.mailbox[sq_idx(rook_src)] = Piece::None;
                current.mailbox[sq_idx(rook_dst)] = rook_piece;

                current.hashes.full_hash ^= hash::castling_rights[current.allowed_castles.as_mask()];
                current.allowed_castles.rooks[us].clear();
                current.hashes.full_hash ^= hash::castling_rights[current.allowed_castles.as_mask()];

                current.hashes.major_hash ^= 
                    attacker_src_hash ^ attacker_dst_hash ^
                    hash::piecesquares[piecesquare(rook_piece, rook_src, false)] ^
                    hash::piecesquares[piecesquare(rook_piece, rook_dst, false)]; 
                current.hashes.minor_hash ^= attacker_src_hash ^ attacker_dst_hash; 

                if (current.stm) current.hashes.non_pawn_black_hash ^= 
                    attacker_src_hash ^ attacker_dst_hash ^
                    hash::piecesquares[piecesquare(rook_piece, rook_src, false)] ^
                    hash::piecesquares[piecesquare(rook_piece, rook_dst, false)];
                else current.hashes.non_pawn_white_hash ^= 
                    attacker_src_hash ^ attacker_dst_hash ^
                    hash::piecesquares[piecesquare(rook_piece, rook_src, false)] ^
                    hash::piecesquares[piecesquare(rook_piece, rook_dst, false)];

                dst = src;
                break;
            }

            case MoveType::EnPassant: {
                int capture_idx = sq_idx(sq_dst) + ((side == Color::White) ? -8 : 8);
                uint64_t bb_cap = (uint64_t)1 << capture_idx;
                Piece captured_pawn = piece_type_with_color(PieceType::Pawn, flip(side));

                current.hashes.full_hash ^= 
                    hash::piecesquares[piecesquare(captured_pawn, sq_from_idx(capture_idx), false)] ^
                    hash::piecesquares[piecesquare(src, sq_dst, false)];
                current.hashes.pawn_hash ^= 
                    attacker_src_hash ^ attacker_dst_hash ^
                    hash::piecesquares[piecesquare(captured_pawn, sq_from_idx(capture_idx), false)];

                current.bitboards[piece_type_idx(PieceType::Pawn)] ^= bb_src ^ bb_dst ^ bb_cap;
                current.bitboards[us + COLOR_OFFSET] ^= bb_src ^ bb_dst;
                current.bitboards[them + COLOR_OFFSET] ^= bb_cap;
                current.mailbox[capture_idx] = Piece::None;
                dst = src;

                break;
            }

            case MoveType::Promotion: {
                if (dst != Piece::None) {
                    current.hashes.full_hash ^= victim_hash;
                    current.bitboards[piece_type_idx(piece_type(dst))] ^= bb_dst;
                    current.bitboards[them + COLOR_OFFSET] ^= bb_dst;

                    if (piece_type(dst) == PieceType::Rook) {
                        current.hashes.full_hash ^= hash::castling_rights[current.allowed_castles.as_mask()];
                        auto& rooks = current.allowed_castles.rooks[them];
                        if (sq_dst == rooks.kingside) rooks.unset(true);
                        else if (sq_dst == rooks.queenside) rooks.unset(false);
                        current.hashes.full_hash ^= hash::castling_rights[current.allowed_castles.as_mask()];
                    }

                    switch (piece_type(dst)) {
                        case PieceType::Pawn: current.hashes.pawn_hash ^= victim_hash; break;
                        case PieceType::Knight: case PieceType::Bishop: current.hashes.minor_hash ^= victim_hash; break;
                        case PieceType::Rook: case PieceType::Queen: current.hashes.major_hash ^= victim_hash; break;
                        case PieceType::King: current.hashes.major_hash ^= victim_hash; current.hashes.minor_hash ^= victim_hash; break;
                        default: break;
                    }

                    if (!current.stm) current.hashes.non_pawn_black_hash ^= victim_hash;
                    else current.hashes.non_pawn_white_hash ^= victim_hash;
                }

                PieceType promo_type = move.promo_piece_type();
                Piece promo_piece = piece_type_with_color(promo_type, side);
                uint64_t promo_hash = hash::piecesquares[piecesquare(promo_piece, sq_dst, false)];

                current.hashes.full_hash ^= promo_hash;
                current.hashes.pawn_hash ^= attacker_src_hash;
                
                if (promo_type <= PieceType::Bishop) current.hashes.minor_hash ^= promo_hash;
                else current.hashes.major_hash ^= promo_hash;

                if (current.stm) current.hashes.non_pawn_black_hash ^= promo_hash;
                else current.hashes.non_pawn_white_hash ^= promo_hash;

                current.bitboards[piece_type_idx(promo_type)] ^= bb_dst;
                current.bitboards[piece_type_idx(PieceType::Pawn)] ^= bb_src;
                current.bitboards[us + COLOR_OFFSET] ^= bb_src ^ bb_dst;
                dst = promo_piece;
                break;
            }

            case MoveType::None: break;
        }
        

        src = Piece::None;
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

    bool Position::is_threefold() {
        uint8_t rep_counter = 1;
        for (PositionState prev_state : history) {
            if (prev_state.hashes.full_hash == current.hashes.full_hash) {
                rep_counter++;
                if (rep_counter == 3) return true;
            }
        }
        return false;
    }

    bool Position::is_insufficient() {
        if (current.bitboards[piece_type_idx(PieceType::Pawn)]) return false;
        if (current.bitboards[piece_type_idx(PieceType::Queen)] | current.bitboards[piece_type_idx(PieceType::Rook)]) return false;
        if (
            (current.bitboards[piece_type_idx(PieceType::Bishop)] & current.bitboards[color_idx(Color::White) + COLOR_OFFSET]) &&
            (current.bitboards[piece_type_idx(PieceType::Bishop)] & current.bitboards[color_idx(Color::Black) + COLOR_OFFSET])
        ) return false;
        if (current.bitboards[piece_type_idx(PieceType::Bishop)] && current.bitboards[piece_type_idx(PieceType::Knight)]) return false;
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
                        case PieceType::Pawn:   c = 'p'; break;
                        case PieceType::Knight: c = 'n'; break;
                        case PieceType::Bishop: c = 'b'; break;
                        case PieceType::Rook:   c = 'r'; break;
                        case PieceType::Queen:  c = 'q'; break;
                        case PieceType::King:   c = 'k'; break;
                        default: c = '?';
                    }
                    fen += (col == Color::White) ? std::toupper(c) : c;
                }
            }
            if (empty != 0)
                fen += std::to_string(empty);
            if (rank != 0)
                fen += '/';
        }
    
        fen += (current.stm == static_cast<bool>(Color::White)) ? " w " : " b ";
    
        std::string castling;
        if (current.allowed_castles.rooks[color_idx(Color::White)].is_kingside_set()) castling += 'K';
        if (current.allowed_castles.rooks[color_idx(Color::White)].is_queenside_set()) castling += 'Q';
        if (current.allowed_castles.rooks[color_idx(Color::Black)].is_kingside_set()) castling += 'k';
        if (current.allowed_castles.rooks[color_idx(Color::Black)].is_queenside_set()) castling += 'q';
        fen += (castling.empty() ? "-" : castling) + " ";
    
        fen += (current.ep_square == Square::None ? "-" : Move(current.ep_square, current.ep_square).to_string().substr(2)) + " ";
    
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
                    if (color(piece) == Color::Black) hashes.non_pawn_black_hash ^= piece_hash;
                    else hashes.non_pawn_white_hash ^= piece_hash;
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
        if (current.ep_square != Square::None) hashes.full_hash ^= hash::ep_files[file(current.ep_square)];
        hashes.full_hash ^= hash::castling_rights[current.allowed_castles.as_mask()];

        return hashes;
    }

    Move from_UCI(const Position& position, const std::string& move) {
        std::string src_str = move.substr(0, 2);
        std::string dst_str = move.substr(2, 2);

        auto str2Sq = [](const std::string& square) {
            int file = square[0] - 'a';
            int rank = square[1] - '1';
            return static_cast<Square>(rank * 8 + file);
        };

        Square src = str2Sq(src_str);
        Square dst = str2Sq(dst_str);

        auto is_castling = [&]() {
            Color stm = position.STM();
            if (piece_type(position.mailbox(sq_idx(src))) != PieceType::King) return false;
        
            Square kingside = (stm == Color::White) ? Square::G1 : Square::G8;
            Square queenside = (stm == Color::White) ? Square::C1 : Square::C8;
        
            bool kingside_castle = (dst == kingside) && position.castling_rights(stm).is_kingside_set();
            bool queenside_castle = (dst == queenside) && position.castling_rights(stm).is_queenside_set();
        
            return (kingside_castle || queenside_castle);
        };

        bool is_promo = (move.length() == 5);
        bool is_en_passant = (piece_type(position.mailbox(sq_idx(src))) == PieceType::Pawn) && (dst == position.ep_square());

        auto char2Piece = [&](char promo) {
            switch (promo) {
                case ('q'): return PromoPiece::Queen;
                case ('r'): return PromoPiece::Rook;
                case ('b'): return PromoPiece::Bishop;
                case ('n'): return PromoPiece::Knight;
                default: return PromoPiece::None;
            }
        };

        if (is_castling()) return Move(src, dst, MoveType::Castling);
        else if (is_en_passant) return Move(src, dst, MoveType::EnPassant);
        else if (is_promo) return Move(src, dst, MoveType::Promotion, char2Piece(move.at(4)));
        else return Move(src, dst);
    }
}
