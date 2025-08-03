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
#include <src/tbprobe.h>
#include <sstream>
#include <utility>
#include <vector>

class position_t
{
    class state_t
    {
      public:
        [[nodiscard]] square_t ep_square() const { return m_ep_square; }
        [[nodiscard]] piece_t  taken() const { return m_taken; }

        [[nodiscard]] all_colors<bitboard_t> checkers() const { return m_checkers; }
        [[nodiscard]] all_colors<bitboard_t> blockers() const { return m_blockers; }
        [[nodiscard]] all_colors<bitboard_t> check_mask() const { return m_check_mask; }
        [[nodiscard]] castling_rights_t      crs() const { return m_crs; }
        [[nodiscard]] zobrist_t              hash() const { return m_hash; }
        [[nodiscard]] int                    halfmove_clock() const { return m_halfmove_clock; }

        void crs_remove_rights(const uint8_t lost_mask) { m_crs.remove_right(lost_mask); }

        state_t()
            : m_ep_square(NO_SQUARE), m_crs(castling_rights_t{0}), m_hash(zobrist_t{0}), m_taken(NO_PIECE),
              m_halfmove_clock(0)
        {
        }
        state_t(const state_t& prev)
            : m_ep_square(NO_SQUARE), m_crs(prev.m_crs), m_hash(prev.m_hash), m_taken(NO_PIECE),
              m_halfmove_clock(prev.m_halfmove_clock)
        {
        }

      private:
        square_t               m_ep_square;
        all_colors<bitboard_t> m_checkers{};
        all_colors<bitboard_t> m_blockers{};
        all_colors<bitboard_t> m_check_mask{};
        castling_rights_t      m_crs;
        zobrist_t              m_hash;
        piece_t                m_taken;
        int                    m_halfmove_clock;

        friend class position_t;
    };

  public:
    // board getters
    [[nodiscard]] std::span<const piece_t> pieces() const { return m_pieces; }
    [[nodiscard]] piece_t                  piece_at(const square_t sq) const { return m_pieces.at(sq); }
    [[nodiscard]] piece_type_t piece_type_at(const square_t sq) const { return piece_piece_type(m_pieces.at(sq)); }
    [[nodiscard]] color_t      color_at(const square_t sq) const { return piece_color(m_pieces.at(sq)); }
    [[nodiscard]] bool         is_occupied(const square_t sq) const { return m_pieces.at(sq) != NO_PIECE; }
    template <class... Ts>
    bitboard_t pieces_bb(color_t c, piece_type_t first, const Ts... rest) const;
    template <class... Ts>
    bitboard_t             pieces_bb(piece_type_t first, const Ts... rest) const;
    [[nodiscard]] square_t ksq(const color_t c) const
    {
        return static_cast<square_t>(get_lsb(m_pieces_occupancy.at(c).at(KING)));
    }
    [[nodiscard]] int full_move() const { return m_fullmove; }
    [[nodiscard]] int ply() const { return m_ply; }

    // state getters
    [[nodiscard]] color_t           color() const { return m_color; }
    [[nodiscard]] bitboard_t        pieces_occupancy(color_t c, piece_type_t p) const;
    [[nodiscard]] bitboard_t        color_occupancy(color_t c) const;
    [[nodiscard]] bitboard_t        occupancy() const { return m_global_occupancy; }
    [[nodiscard]] const state_t&    state() const { return m_states.at(m_state_idx); }
    [[nodiscard]] bitboard_t        checkers(const color_t c) const { return state().checkers().at(c); }
    [[nodiscard]] bitboard_t        blockers(const color_t c) const { return state().blockers().at(c); }
    [[nodiscard]] bitboard_t        check_mask(color_t c) const;
    [[nodiscard]] square_t          ep_square() const { return state().ep_square(); }
    [[nodiscard]] hash_t            hash() const { return state().hash().value(); }
    [[nodiscard]] castling_rights_t crs() const { return state().crs(); }
    [[nodiscard]] int               halfmove_clock() const { return state().halfmove_clock(); }

    // relative directions
    [[nodiscard]] direction_t up() const { return color() == WHITE ? NORTH : SOUTH; }
    [[nodiscard]] direction_t down() const { return static_cast<direction_t>(-up()); }
    [[nodiscard]] direction_t right() const { return color() == BLACK ? EAST : WEST; }
    [[nodiscard]] direction_t left() const { return static_cast<direction_t>(-right()); }
    [[nodiscard]] direction_t up_right() const { return static_cast<direction_t>(up() + right()); }
    [[nodiscard]] direction_t up_left() const { return static_cast<direction_t>(up() + left()); }
    [[nodiscard]] direction_t down_right() const { return static_cast<direction_t>(down() + right()); }
    [[nodiscard]] direction_t down_left() const { return static_cast<direction_t>(down() + left()); }

    // utilities
    [[nodiscard]] bitboard_t attacking_sq_bb(square_t sq) const;
    template <color_t c>
    [[nodiscard]] bool is_legal(move_t move) const;

    template <piece_type_t pt>
    void update_checkers(color_t c);
    void update_checkers(color_t c);
    [[nodiscard]] bool gives_check(move_t move) const;

    void set_piece(piece_t piece, square_t sq);
    void set_piece(piece_type_t piece_type, color_t color, square_t sq);

    void remove_piece(piece_type_t piece_type, color_t color, square_t sq);
    void remove_piece(piece_t piece, square_t sq);

    void move_piece(piece_type_t piece_type, color_t c, square_t from, square_t to);
    void move_piece(piece_t piece, square_t from, square_t to);

    void do_move(move_t move);
    void do_null_move();
    void undo_move(move_t move);
    void undo_null_move();

    void init_state();
    void from_fen(std::string_view fen);

    [[nodiscard]] bool is_draw() const;
    unsigned           wdl_probe() const;
    unsigned           dtz_probe() const;

    void push_state()
    {
        m_states.at(m_state_idx + 1) = state_t(m_states.at(m_state_idx));
        m_state_idx++;
    }

    void pop_state() { m_state_idx--; }

    [[nodiscard]] std::string to_string() const;
    friend std::ostream&      operator<<(std::ostream& os, const position_t& pos) { return os << pos.to_string(); }

  private:
    all_squares<piece_t>                    m_pieces{};
    all_colors<all_piece_types<bitboard_t>> m_pieces_occupancy{};
    all_colors<bitboard_t>                  m_color_occupancy{};
    bitboard_t                              m_global_occupancy{};
    color_t                                 m_color{};
    int                                     m_ply      = 0;
    int                                     m_fullmove = 1;

    int                      m_state_idx = 0;
    std::array<state_t, 256> m_states;
};

template <typename... Ts>
[[nodiscard]] bitboard_t position_t::pieces_bb(const color_t c, const piece_type_t first, const Ts... rest) const
{
    if constexpr (sizeof...(rest) == 0)
    {
        return m_pieces_occupancy.at(c).at(first);
    }
    else
    {
        return m_pieces_occupancy.at(c).at(first) | pieces_bb(c, rest...);
    }
}
template <typename... Ts>
bitboard_t position_t::pieces_bb(const piece_type_t first, const Ts... rest) const
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
    return state().check_mask().at(c);
}

template <piece_type_t pt>
void position_t::update_checkers(const color_t c)
{
    auto enemies = pieces_bb(~c, pt);
    while (enemies)
    {
        const auto sq         = static_cast<square_t>(pop_lsb(enemies));
        const auto line       = bb::from_to_excl(sq, ksq(c)) & bb::attacks<pt>(sq);
        const auto on_line    = line & occupancy();
        const auto n_blockers = popcount(on_line);
        if (n_blockers == 0)
        {
            // check mask also contains all squares between the long range attacker and the king
            m_states.at(m_state_idx).m_check_mask.at(c) |= line;
        }
        if (n_blockers == 1)
        {
            // blockers can be of any color
            // blocker from same color are pinned from other color can do a discovered check
            m_states.at(m_state_idx).m_blockers.at(c) |= on_line;
        }
    }
}

inline void position_t::update_checkers(const color_t c)
{
    const auto ksq = static_cast<square_t>(get_lsb(pieces_bb(c, KING)));

    m_states.at(m_state_idx).m_blockers.at(c)   = bb::empty;
    m_states.at(m_state_idx).m_check_mask.at(c) = m_states.at(m_state_idx).m_checkers.at(c) =
        attacking_sq_bb(ksq) & color_occupancy(~c);

    update_checkers<BISHOP>(c);
    update_checkers<ROOK>(c);
    update_checkers<QUEEN>(c);
}

inline bool position_t::gives_check(const move_t move) const
{
    assert(piece_color(piece_at(move.from_sq())) == color());
    return (bb::attacks(piece_type_at(move.from_sq()), move.to_sq()) & ksq(~color())) != bb::empty;
}

inline void position_t::set_piece(const piece_t piece, const square_t sq)
{
    assert(!is_occupied(sq));
    const piece_type_t pt = piece_piece_type(piece);
    const color_t      c  = piece_color(piece);
    m_global_occupancy |= m_color_occupancy.at(c) |= m_pieces_occupancy.at(c).at(pt) |= bb::sq_mask(sq);
    m_pieces.at(sq) = piece;
}

inline void position_t::set_piece(const piece_type_t piece_type, const color_t color, const square_t sq)
{
    assert(!is_occupied(sq));
    m_global_occupancy |= m_color_occupancy.at(color) |= m_pieces_occupancy.at(color).at(piece_type) |= bb::sq_mask(sq);
    m_pieces.at(sq) = piece(piece_type, color);
}

inline void position_t::remove_piece(const piece_type_t piece_type, const color_t color, const square_t sq)
{
    assert(piece_at(sq) == piece(piece_type, color));
    m_pieces_occupancy.at(color).at(piece_type) &= m_color_occupancy.at(color) &= m_global_occupancy &=
        ~bb::sq_mask(sq);
    m_pieces.at(sq) = NO_PIECE;
}

inline void position_t::remove_piece(const piece_t piece, const square_t sq)
{
    const piece_type_t pt = piece_piece_type(piece);
    const color_t      c  = piece_color(piece);
    assert(piece_at(sq) == piece);
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

inline void position_t::init_state()
{
    m_states.at(m_state_idx).m_hash = zobrist_t(*this);

    update_checkers(WHITE);
    update_checkers(BLACK);
}

inline void position_t::from_fen(const std::string_view fen)
{
    m_pieces.fill(NO_PIECE);
    m_color_occupancy.fill(bb::empty);
    m_global_occupancy = bb::empty;
    m_pieces_occupancy.fill(all_piece_types<bitboard_t>{});
    m_states.at(0) = state_t();

    std::istringstream iss{std::string(fen)};
    std::string        board, color, castling, ep, halfmove, fullmove;
    iss >> board >> color >> castling >> ep >> halfmove >> fullmove;

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
            set_piece(char_to_piece(c), square(file, rank));

            file = static_cast<file_t>(file + 1);
        }
    }

    assert(color.size() == 1);
    m_color = char_to_color(color[0]);

    castling_rights_t& crs = m_states.at(m_state_idx).m_crs;
    crs                    = castling_rights_t::from_string(castling);

    m_states.at(m_state_idx).m_ep_square = string_to_square(ep);

    assert(halfmove.size() <= 3);
    m_states.at(m_state_idx).m_halfmove_clock = stoi(halfmove);

    assert(fullmove.size() < 3);
    m_fullmove = stoi(fullmove);

    init_state();
}

inline std::string position_t::to_string() const
{
    std::ostringstream out;

    out << "Position:\n";
    out << "Side to move: " << color() << "\n";
    out << "Castling rights: " << m_states.at(m_state_idx).m_crs << "\n";

    out << "En passant square: " << ep_square() << "\n";
    ;

    out << "Zobrist hash: 0x" << std::hex << state().hash().value() << std::dec << "\n";

    for (rank_t rank = RANK_8; rank >= RANK_1; rank = static_cast<rank_t>(rank - 1))
    {
        out << rank + 1 << " ";
        for (const auto file : files)
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

    if (piece_type_at(move.from_sq()) == KING)
    {
        bitboard_t long_range = checkers(c) & pieces_bb(~c, ROOK, BISHOP, QUEEN);
        while (long_range)
        {
            if (const auto sq = static_cast<square_t>(pop_lsb(long_range));
                bb::are_aligned(sq, move.from_sq(), move.to_sq()) && move.to_sq() != sq)
            {
                return false;
            }
        }
        if ((attacking_sq_bb(move.to_sq()) & color_occupancy(~c)) != bb::empty)
        {
            return false;
        }
    }
    if (move.type_of() == NORMAL || move.type_of() == PROMOTION)
    {
        // check for pins
        if ((bb::sq_mask(move.from_sq()) & blockers(c)) != bb::empty)
        {
            if ((bb::sq_mask(move.to_sq()) & bb::line(move.from_sq(), ksq(c))) == bb::empty)
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
            if (pieces_occupancy(~c, piece_type) & bb::attacks(piece_type, ksq(c), occupancy))
            {
                return false;
            }
        }
    }
    return true;
}

inline void position_t::do_move(const move_t move)
{
    if (move == move_t::null())
    {
        do_null_move();
        return;
    }
    push_state();

    state_t& state = m_states.at(m_state_idx);

    if (const square_t prev_ep = m_states.at(m_state_idx - 1).ep_square(); prev_ep != NO_SQUARE)
    {
        state.m_hash.flip_ep(fl_of(prev_ep));
    }

    const square_t from = move.from_sq();
    const square_t to   = move.to_sq();
    piece_type_t   pt   = piece_type_at(from);
    const color_t  us   = color();

    m_color = ~m_color;
    state.m_hash.flip_color();

    state.m_halfmove_clock++;
    if (us == BLACK)
        m_fullmove++;
    m_ply++;

    const direction_t up = us == WHITE ? NORTH : SOUTH;

    const move_type_t move_type = move.type_of();

    const uint8_t lost =
        (castling_rights_t::lost_by_moving_from(from) | castling_rights_t::lost_by_moving_from(to)) & crs().mask();
    if (lost)
    {
        state.m_crs.remove_right(lost);
        state.m_hash.flip_castling_rights(lost);
    }
    if (move_type == CASTLING)
    {
        const auto castling_type = move.castling_type();
        assert(castling_rights_t{lost}.has_right(castling_type));
        auto [k_from, k_to] = castling_rights_t::king_move(castling_type);
        auto [r_from, r_to] = castling_rights_t::rook_move(castling_type);

        remove_piece(ROOK, us, r_from);
        move_piece(KING, us, k_from, k_to);
        set_piece(ROOK, us, r_to);

        state.m_hash.move_piece(piece(KING, us), k_from, k_to);
        state.m_hash.move_piece(piece(ROOK, us), r_from, r_to);

        update_checkers(~us);
        return;
    }

    if (pt == PAWN || is_occupied(to) || move_type == EN_PASSANT)
    {
        state.m_halfmove_clock = 0;
    }

    if (move_type == NORMAL || move_type == PROMOTION)
    {
        // capture
        if (is_occupied(to))
        {
            assert(color_at(to) == ~us);

            state.m_taken = piece_at(to);
            state.m_hash.flip_piece(piece_at(to), to);
            remove_piece(piece_at(to), to);
        }
        // set new ep square
        else if (pt == PAWN && (to - from == (2 * up)))
        {
            if (((bb::pseudo_attack<PAWN>(static_cast<square_t>(to - static_cast<int>(up)), us)) &
                 color_occupancy(~us)) != bb::empty)
            {
                state.m_ep_square = static_cast<square_t>(from + static_cast<int>(up));
                state.m_hash.flip_ep(fl_of(from));
            }
        }
    }
    if (move_type == EN_PASSANT)
    {
        const auto to_ep = static_cast<square_t>(to - static_cast<int>(up));
        state.m_hash.flip_piece(piece(PAWN, ~us), to_ep);
        remove_piece(PAWN, ~us, to_ep);
    }
    if (move_type == PROMOTION)
    {
        remove_piece(PAWN, us, from);
        set_piece(move.promotion_type(), us, from);
        pt = move.promotion_type();
        state.m_hash.promote_piece(us, pt, from);
    }
    move_piece(pt, us, from, to);
    state.m_hash.move_piece(piece(pt, us), from, to);
    update_checkers(~us);
}

inline void position_t::do_null_move()
{
    push_state();

    state_t& state = m_states.at(m_state_idx);

    const color_t us = color();

    if (const square_t prev_ep = m_states.at(m_state_idx - 1).ep_square(); prev_ep != NO_SQUARE)
    {
        state.m_hash.flip_ep(fl_of(prev_ep));
    }

    state.m_ep_square = NO_SQUARE;

    m_color = ~m_color;
    state.m_hash.flip_color();

    state.m_halfmove_clock++;
    if (us == BLACK)
        m_fullmove++;

    m_ply++;

    update_checkers(~us);
}

inline void position_t::undo_move(const move_t move)
{

    if (move == move_t::null())
    {
        undo_null_move();
        return;
    }

    const piece_t taken = state().m_taken;
    pop_state();

    m_color = ~m_color;

    const square_t     from      = move.from_sq();
    const square_t     to        = move.to_sq();
    const piece_type_t pt        = piece_type_at(to);
    const color_t      us        = color();
    const direction_t  up        = us == WHITE ? NORTH : SOUTH;
    const move_type_t  move_type = move.type_of();

    m_ply--;
    if (us == BLACK)
        m_fullmove--;

    if (move_type == CASTLING)
    {
        const auto castling_type = move.castling_type();
        auto [k_from, k_to]      = castling_rights_t::king_move(castling_type);
        auto [r_from, r_to]      = castling_rights_t::rook_move(castling_type);

        remove_piece(ROOK, us, r_to);
        move_piece(KING, us, k_to, k_from);
        set_piece(ROOK, us, r_from);

        return;
    }

    move_piece(pt, us, to, from);

    if (move_type == PROMOTION)
    {
        if (taken != NO_PIECE)
        {
            assert(piece_color(taken) == ~us);
            set_piece(taken, to);
        }
        remove_piece(move.promotion_type(), us, from);
        set_piece(PAWN, us, from);
    }
    if (move_type == EN_PASSANT)
    {
        // remove two up right / left
        set_piece(PAWN, ~us, static_cast<square_t>(static_cast<int>(to) - up));
    }
    if (move_type == NORMAL)
    {
        // capture
        if (taken != NO_PIECE)
        {
            assert(piece_color(taken) == ~us);
            set_piece(taken, to);
        }
    }
}

inline void position_t::undo_null_move()
{
    pop_state();
    m_color = ~m_color;

    m_ply--;
    if (m_color == BLACK)
        m_fullmove--;

    update_checkers(~m_color);
}

inline bool position_t::is_draw() const
{
    if (halfmove_clock() >= 100)
    {
        return true;
    }
    const hash_t target = hash();
    int          hits   = 1;
    size_t       idx    = m_state_idx;
    while (idx > 0)
    {
        state_t state = m_states.at(--idx);
        if (state.halfmove_clock() == 0 && idx != 0)
        {
            return false;
        }
        if (state.hash().value() == target)
        {
            hits++;
        }
        if (hits >= 3)
        {
            return true;
        }
    }
    return false;
}

inline unsigned position_t::wdl_probe() const
{
        int epsq = ep_square() == NO_SQUARE ? 0 : ep_square() + 1;
        return tb_probe_wdl(color_occupancy(WHITE), color_occupancy(BLACK), pieces_bb(KING), pieces_bb(QUEEN),
                            pieces_bb(ROOK), pieces_bb(BISHOP), pieces_bb(KNIGHT), pieces_bb(PAWN),
                            static_cast<unsigned>(halfmove_clock()), crs().mask(), 0,
                            color() == WHITE);
}

inline unsigned position_t::dtz_probe() const
{
    int epsq = ep_square() == NO_SQUARE ? 0 : ep_square() + 1;
    return tb_probe_root(color_occupancy(WHITE), color_occupancy(BLACK), pieces_bb(KING), pieces_bb(QUEEN),
                        pieces_bb(ROOK), pieces_bb(BISHOP), pieces_bb(KNIGHT), pieces_bb(PAWN),
                        static_cast<unsigned>(halfmove_clock()), crs().mask(), epsq,
                        color() == WHITE, nullptr);
}


#endif
