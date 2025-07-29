#ifndef POSITION_H_INCLUDED
#define POSITION_H_INCLUDED
#include "bitboard.h"
#include "castle.h"
#include "move.h"
#include "prng.h"
#include "types.h"
#include "zobrist.h"

#include <iostream>
#include <memory>
#include <span>
#include <sstream>
#include <utility>
#include <vector>

class state_t
{
  public:
    state_t() : m_ep_square(NO_SQUARE), m_crs(castling_rights_t{0}), m_hash(zobrist_t{0}), m_taken(NO_PIECE) {}
    explicit state_t(const state_t* prev)
        : m_ep_square(NO_SQUARE), m_crs(prev->m_crs), m_hash(prev->m_hash), m_taken(NO_PIECE)
    {
    }
    square_t               m_ep_square;
    all_colors<bitboard_t> m_checkers{};
    all_colors<bitboard_t> m_blockers{};
    all_colors<bitboard_t> m_check_mask{};
    castling_rights_t      m_crs;
    zobrist_t              m_hash;
    piece_t                m_taken;
};

class position_t
{
  public:
    all_squares<piece_t>                    m_pieces{};
    all_colors<all_piece_types<bitboard_t>> m_pieces_occupancy{};
    all_colors<bitboard_t>                  m_color_occupancy{};
    bitboard_t                              m_global_occupancy{};
    color_t                                 m_color{};
    int                                     m_state_idx = 0;
    state_t*                                m_state{};
    std::array<state_t, 256>                m_states;

    [[nodiscard]] std::span<const piece_t> pieces() const { return m_pieces; }
    [[nodiscard]] piece_t                  piece_at(const square_t sq) const { return m_pieces.at(sq); }
    [[nodiscard]] piece_type_t piece_type_at(const square_t sq) const { return piece_piece_type(m_pieces.at(sq)); }
    [[nodiscard]] bool         is_occupied(const square_t sq) const
    {
        return (m_global_occupancy & bb::sq_mask(sq)) != bb::empty;
    }
    [[nodiscard]] color_t            color() const { return m_color; }
    [[nodiscard]] bitboard_t         pieces_occupancy(color_t c, piece_type_t p) const;
    [[nodiscard]] bitboard_t         color_occupancy(color_t c) const;
    [[nodiscard]] bitboard_t         checkers(const color_t c) const { return m_state->m_checkers.at(c); }
    [[nodiscard]] bitboard_t         blockers(const color_t c) const { return m_state->m_blockers.at(c); }
    [[nodiscard]] bitboard_t         check_mask(color_t c) const;
    [[nodiscard]] square_t           ep_square() const { return m_state->m_ep_square; }
    [[nodiscard]] castling_rights_t& crs() const { return m_state->m_crs; }
    template <class... Ts>
    bitboard_t pieces_bb(color_t c, piece_type_t first, const Ts... rest) const;
    template <class... Ts>
    bitboard_t pieces_bb(piece_type_t first, const Ts... rest) const;

    [[nodiscard]] direction_t up() const { return color() == WHITE ? NORTH : SOUTH; }
    [[nodiscard]] direction_t down() const { return static_cast<direction_t>(-up()); }
    [[nodiscard]] direction_t right() const { return color() == BLACK ? EAST : WEST; }
    [[nodiscard]] direction_t left() const { return static_cast<direction_t>(-right()); }
    [[nodiscard]] direction_t up_right() const { return static_cast<direction_t>(up() + right()); }
    [[nodiscard]] direction_t up_left() const { return static_cast<direction_t>(up() + left()); }
    [[nodiscard]] direction_t down_right() const { return static_cast<direction_t>(down() + right()); }
    [[nodiscard]] direction_t down_left() const { return static_cast<direction_t>(down() + left()); }

    [[nodiscard]] bitboard_t attacking_sq_bb(square_t sq) const;

    template <piece_type_t pt>
    void                      update_checkers(color_t c) const;
    void                      update_checkers(color_t c) const;
    void                      set_piece(piece_t piece, square_t sq);
    void                      set_piece(piece_type_t piece_type, color_t color, square_t sq);
    void                      remove_piece(piece_type_t piece_type, color_t color, square_t sq);
    void                      remove_piece(piece_t piece, square_t sq);
    void                      move_piece(piece_type_t piece_type, color_t c, square_t from, square_t to);
    void                      move_piece(piece_t piece, square_t from, square_t to);
    void                      init_state() const;
    void                      from_fen(std::string_view fen);
    [[nodiscard]] std::string to_string() const;
    template <color_t c>
    [[nodiscard]] bool   is_legal(move_t move) const;
    void                 do_move(move_t move);
    void                 undo_move(move_t move);
    friend std::ostream& operator<<(std::ostream& os, const position_t& pos) { return os << pos.to_string(); }
};

template <typename... Ts>
inline bitboard_t position_t::pieces_bb(const color_t c, const piece_type_t first, const Ts... rest) const
{
    if constexpr (sizeof...(rest) == 0)
    {
        return m_pieces_occupancy[c][first];
    }
    else
    {
        return m_pieces_occupancy[c][first] | get_pieces_bitboard(c, rest...);
    }
}
template <typename... Ts>
inline bitboard_t position_t::pieces_bb(const piece_type_t first, const Ts... rest) const
{
    return pieces_bb(WHITE, first, rest...) | pieces_bb(BLACK, first, rest...);
}
inline bitboard_t position_t::attacking_sq_bb(const square_t sq) const
{
    // if a piece attacks a square sq2 from a square sq1 it would attack sq1 from sq2
    // exception made for pawns where this is true only with reversed colors
    return (bb::attacks<QUEEN>(sq, m_global_occupancy) & pieces_bb(QUEEN)) |
           (bb::attacks<ROOK>(sq, m_global_occupancy) & pieces_bb(ROOK)) |
           (bb::attacks<BISHOP>(sq, m_global_occupancy) & pieces_bb(BISHOP)) |
           (bb::attacks<KNIGHT>(sq, m_global_occupancy) & pieces_bb(KNIGHT)) |
           (bb::attacks<PAWN>(sq, m_global_occupancy, BLACK) & pieces_bb(WHITE, PAWN)) |
           (bb::attacks<PAWN>(sq, m_global_occupancy, WHITE) & pieces_bb(BLACK, PAWN)) |
           (bb::attacks<KING>(sq, m_global_occupancy) & pieces_bb(KING));
}
inline bitboard_t position_t::pieces_occupancy(const color_t c, const piece_type_t p) const
{
    return m_pieces_occupancy.at(c).at(p);
}
inline bitboard_t position_t::color_occupancy(const color_t c) const
{
    if (c == ANY)
    {
        return m_global_occupancy;
    }
    return m_color_occupancy.at(c);
}
inline bitboard_t position_t::check_mask(const color_t c) const
{
    return m_state->m_check_mask.at(c);
}

template <piece_type_t pt>
void position_t::update_checkers(const color_t c) const
{
    const auto ksq = static_cast<square_t>(get_lsb(pieces_bb(c, KING)));

    auto enemies = pieces_bb(~c, pt);
    while (enemies)
    {
        const auto sq         = static_cast<square_t>(pop_lsb(enemies));
        const auto line       = bb::from_to_excl(sq, ksq) & bb::attacks<pt>(sq);
        const auto on_line    = line & color_occupancy(ANY);
        const auto n_blockers = popcount(on_line);
        if (n_blockers == 0)
        {
            // check mask also contains all squares between the long range attacker and the king
            m_state->m_check_mask.at(c) |= line;
        }
        if (n_blockers == 1)
        {
            // blockers can be of any color
            // blocker from same color are pinned from other color can do a discovered check
            m_state->m_blockers.at(c) |= on_line;
        }
    }
}

inline void position_t::update_checkers(const color_t c) const
{
    const auto ksq = static_cast<square_t>(get_lsb(pieces_bb(c, KING)));
    // set checkers
    // check mask contains checkers that can be captures to resolve check
    m_state->m_check_mask.at(c) = m_state->m_checkers.at(c) = (attacking_sq_bb(ksq) & color_occupancy(~c));
    m_state->m_blockers.at(c)                               = bb::empty;
    update_checkers<BISHOP>(c);
    update_checkers<ROOK>(c);
    update_checkers<QUEEN>(c);
}

inline void position_t::set_piece(const piece_t piece, const square_t sq)
{
    const piece_type_t pt = piece_piece_type(piece);
    const color_t      c  = piece_color(piece);
    m_global_occupancy |= m_color_occupancy.at(c) |= m_pieces_occupancy.at(c).at(pt) |= bb::sq_mask(sq);
    m_pieces.at(sq) = piece;
}

inline void position_t::set_piece(const piece_type_t piece_type, const color_t color, const square_t sq)
{
    m_global_occupancy |= m_color_occupancy.at(color) |= m_pieces_occupancy.at(color).at(piece_type) |= bb::sq_mask(sq);
    m_pieces.at(sq) = piece(piece_type, color);
}

inline void position_t::remove_piece(const piece_type_t piece_type, const color_t color, const square_t sq)
{
    assert(piece_at(sq) == piece(piece_type, color));
    // in this order
    m_pieces_occupancy.at(color).at(piece_type) &= m_color_occupancy.at(color) &= m_global_occupancy &=
        ~bb::sq_mask(sq);
    m_pieces.at(sq) = NO_PIECE;
}

inline void position_t::remove_piece(const piece_t piece, const square_t sq)
{
    const piece_type_t pt = piece_piece_type(piece);
    const color_t      c  = piece_color(piece);
    assert(piece_at(sq) == piece);
    // in this order
    m_pieces_occupancy.at(c).at(pt) &= m_color_occupancy.at(c) &= m_global_occupancy &= ~bb::sq_mask(sq);
    m_pieces.at(sq) = NO_PIECE;
}

inline void position_t::move_piece(const piece_type_t piece_type, const color_t c, const square_t from,
                                   const square_t to)
{
    remove_piece(piece_type, c, from);
    set_piece(piece_type, c, to);
}

inline void position_t::move_piece(const piece_t piece, const square_t from, const square_t to)
{
    remove_piece(piece, from);
    set_piece(piece, to);
}

inline void position_t::init_state() const
{
    m_state->m_hash = zobrist_t(*this);
    update_checkers(color());
    update_checkers(~color());
}

inline piece_type_t piece_type_from_char(const char c)
{
    switch (c)
    {
        case 'P':
        case 'p':
            return PAWN;
        case 'N':
        case 'n':
            return KNIGHT;
        case 'B':
        case 'b':
            return BISHOP;
        case 'R':
        case 'r':
            return ROOK;
        case 'Q':
        case 'q':
            return QUEEN;
        case 'K':
        case 'k':
            return KING;
        default:
            assert(0 && "Invalid piece character");
            return NO_PIECE_TYPE;
    }
}

inline square_t square_from_string(const std::string_view s)
{
    if (s.size() != 2 || s[0] < 'a' || s[0] > 'h' || s[1] < '1' || s[1] > '8')
        return NO_SQUARE;
    const auto file = static_cast<file_t>(s[0] - 'a');
    const auto rank = static_cast<rank_t>(s[1] - '1');
    return square(file, rank);
}

inline std::string square_to_string(const square_t sq)
{
    return std::string{static_cast<char>('a' + fl_of(sq)), static_cast<char>('1' + rk_of(sq))};
}

inline char piece_to_char(const color_t color, const piece_type_t pt)
{
    static all_colors<all_piece_types<char>> pieces = {{'P', 'N', 'B', 'R', 'Q', 'K', 'p', 'n', 'b', 'r', 'q', 'k'}};
    return pieces.at(color).at(pt);
}

inline char piece_to_char(const piece_t pc)
{
    static all_pieces<char> pieces = {{'P', 'N', 'B', 'R', 'Q', 'K', 'p', 'n', 'b', 'r', 'q', 'k'}};
    return pieces.at(pc);
}

inline void position_t::from_fen(const std::string_view fen)
{
    m_pieces.fill(NO_PIECE);
    m_color_occupancy.fill(bb::empty);
    m_global_occupancy = bb::empty;
    m_pieces_occupancy.fill(all_piece_types<bitboard_t>{});
    m_states.at(0) = state_t();
    m_state        = &m_states.at(0);

    std::istringstream iss{std::string(fen)};
    std::string        board, color, castling, ep;
    iss >> board >> color >> castling >> ep;

    file_t file = FILE_A;
    rank_t rank = RANK_8;
    for (const auto c : board)
    {
        if (c == '/')
        {
            rank = static_cast<rank_t>(rank - 1);
            file = FILE_A;
        }
        else if (std::isdigit(c))
        {
            file = static_cast<file_t>(file + c - '0');
        }
        else
        {
            const square_t     sq         = square(file, rank);
            const color_t      turn       = std::isupper(c) ? WHITE : BLACK;
            const piece_type_t piece_type = piece_type_from_char(c);
            set_piece(piece_type, turn, sq);
            file = static_cast<file_t>(file + 1);
        }
    }

    m_color = color == "w" ? WHITE : BLACK;

    m_state->m_crs = castling_rights_t{0};
    if (castling != "-")
    {
        for (const char c : castling)
        {
            switch (c)
            {
                case 'K':
                    m_state->m_crs.add(WHITE_OO);
                    break;
                case 'Q':
                    m_state->m_crs.add(WHITE_OOO);
                    break;
                case 'k':
                    m_state->m_crs.add(BLACK_OO);
                    break;
                case 'q':
                    m_state->m_crs.add(BLACK_OOO);
                    break;
                default:
                    assert(0 && "Invalid castling character");
            }
        }
    }

    m_state->m_ep_square = (ep == "-") ? NO_SQUARE : square_from_string(ep);

    // Hash + checkers
    init_state();
}

inline std::string position_t::to_string() const
{
    std::ostringstream out;

    out << "Position:\n";
    out << "Side to move: " << (color() == WHITE ? "White" : "Black") << "\n";
    out << "Castling rights: " << m_state->m_crs.to_string() << "\n";

    out << "En passant square: ";
    if (m_state->m_ep_square == NO_SQUARE)
        out << "-\n";
    else
        out << square_to_string(m_state->m_ep_square) << "\n";

    out << "Zobrist hash: 0x" << std::hex << m_state->m_hash.value() << std::dec << "\n";

    // Print board from top (rank 8) to bottom (rank 1)
    for (rank_t rank = RANK_8; rank >= RANK_1; rank = static_cast<rank_t>(rank - 1))
    {
        out << rank + 1 << " ";
        for (file_t file = FILE_A; file <= FILE_H; file = static_cast<file_t>(file + 1))
        {
            const square_t sq = square(file, rank);
            const piece_t  pc = piece_at(sq);
            out << (pc != NO_PIECE ? piece_to_char(pc) : '.') << " ";
        }
        out << "\n";
    }

    out << "  a b c d e f g h\n";
    return out.str();
}

template <color_t c>
bool position_t::is_legal(const move_t move) const
{
    constexpr direction_t down    = c == WHITE ? SOUTH : NORTH;
    const bitboard_t      from_bb = bb::sq_mask(move.from_sq());
    const bitboard_t      to_bb   = bb::sq_mask(move.to_sq());
    const piece_type_t    pt      = piece_type_at(move.from_sq());
    const auto            ksq     = static_cast<square_t>(get_lsb(pieces_occupancy(c, KING)));
    if (pt == KING)
    {
        bitboard_t long_range =
            checkers(c) & (pieces_bb(~c, ROOK) | pieces_occupancy(~c, BISHOP) | pieces_occupancy(~c, QUEEN));
        while (long_range)
        {
            if (const auto sq = static_cast<square_t>(pop_lsb(long_range));
                (bb::line(sq, move.to_sq()) == bb::line(sq, move.from_sq())) && move.to_sq() != sq)
            {
                return false;
            }
        }
        if (attacking_sq_bb(move.to_sq()) & color_occupancy(~c))
        {
            return false;
        }
    }
    if (move.type_of() == NORMAL || move.type_of() == PROMOTION)
    {
        // check for pins
        if (bb::sq_mask(move.from_sq()) & blockers(color()))
        {
            if (!(bb::sq_mask(move.to_sq()) & bb::line(move.from_sq(), ksq)))
            {
                return false;
            }
        }
    }
    if (move.type_of() == EN_PASSANT)
    {

        // en passant can create discovered checks so we check for any long range attack
        // we know they have not moved. therefore we only need to update global occupancy to cast rays
        // we check if rays intersect with any long range and if they do that means a there is a check
        bitboard_t occupancy = color_occupancy(ANY);
        occupancy &= ~from_bb;
        occupancy |= to_bb;
        occupancy &= ~bb::shift<down>(to_bb);
        for (const auto piece_type : {BISHOP, ROOK, QUEEN})
        {
            const bitboard_t rays = bb::attacks(piece_type, ksq, occupancy);
            if (pieces_occupancy(~c, piece_type) & rays)
            {
                return false;
            }
        }
    }
    return true;
}

inline void position_t::do_move(const move_t move)
{
    m_states.at(++m_state_idx) = state_t(m_state);
    m_state                    = &m_states.at(m_state_idx);

    const square_t     from     = move.from_sq();
    const square_t     to       = move.to_sq();
    const piece_type_t pt       = piece_type_at(from);
    const color_t      c        = color();
    m_color                     = ~m_color;
    const direction_t up        = c == WHITE ? NORTH : SOUTH;
    const move_type_t move_type = move.type_of();
    // are we losing castling rights?

    // are we castling?
    crs().remove_right(castling_rights_t::lost_by_moving_from(from));
    crs().remove_right(castling_rights_t::lost_by_moving_from(to));
    if (move_type == CASTLING)
    {
        const auto castling_type = move.castling_type();
        auto [k_from, k_to]      = castling_rights_t::king_move(castling_type);
        auto [r_from, r_to]      = castling_rights_t::rook_move(castling_type);

        // std::cout << "HERE" << this;
        remove_piece(ROOK, c, r_from);
        move_piece(KING, c, k_from, k_to);
        set_piece(ROOK, c, r_to);
        update_checkers(~c);
        return;
    }

    if (move_type == NORMAL)
    {
        // capture
        if (is_occupied(to))
        {
            assert(piece_color(piece_at(to)) == ~c);
            m_state->m_taken = piece_at(to);
            remove_piece(piece_at(to), to);
        }
        // set new ep square
        else if (pt == PAWN && (to - from == (2 * up)))
        {
            m_state->m_ep_square = static_cast<square_t>(from + static_cast<int>(up));
        }
    }
    if (move_type == EN_PASSANT)
    {
        // remove two up right / left
        remove_piece(PAWN, ~c, static_cast<square_t>(to - static_cast<int>(up)));
    }
    if (move_type == PROMOTION)
    {
        if (is_occupied(to))
        {
            assert(piece_color(piece_at(to)) == ~c);
            m_state->m_taken = piece_at(to);
            remove_piece(piece_at(to), to);
        }
        move_piece(pt, c, from, to);

        // remove pawn add promoted type
        remove_piece(PAWN, c, to);
        set_piece(move.promotion_type(), c, to);
    }
    else
    {
        move_piece(pt, c, from, to);
    }
    update_checkers(~c);
}

inline void position_t::undo_move(const move_t move)
{

    const piece_t taken = m_state->m_taken;
    m_state             = &m_states.at(--m_state_idx);
    m_color             = ~m_color;

    const square_t     from      = move.from_sq();
    const square_t     to        = move.to_sq();
    const piece_type_t pt        = piece_type_at(to);
    const color_t      c         = color();
    const direction_t  up        = c == WHITE ? NORTH : SOUTH;
    const move_type_t  move_type = move.type_of();

    if (move_type == CASTLING)
    {
        const auto castling_type = move.castling_type();
        auto [k_from, k_to]      = castling_rights_t::king_move(castling_type);
        auto [r_from, r_to]      = castling_rights_t::rook_move(castling_type);

        remove_piece(ROOK, c, r_to);
        move_piece(KING, c, k_to, k_from);
        set_piece(ROOK, c, r_from);
        return;
    }

    move_piece(pt, c, to, from);
    if (move_type == PROMOTION)
    {
        if (taken != NO_PIECE)
        {
            assert(piece_color(taken) == ~c);
            set_piece(taken, to);
        }
        // remove pawn add promoted type
        remove_piece(move.promotion_type(), c, from);
        set_piece(PAWN, c, from);
    }
    if (move_type == EN_PASSANT)
    {
        // remove two up right / left
        set_piece(PAWN, ~c, static_cast<square_t>(static_cast<int>(to) - up));
    }
    if (move_type == NORMAL)
    {
        // capture
        if (taken != NO_PIECE)
        {
            assert(piece_color(taken) == ~c);
            set_piece(taken, to);
        }
    }
}
#endif
