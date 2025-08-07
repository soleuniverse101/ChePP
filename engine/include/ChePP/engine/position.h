#ifndef POSITION_H_INCLUDED
#define POSITION_H_INCLUDED
#include "bitboard.h"
#include "castle.h"
#include "move.h"
#include "types.h"
#include "zobrist.h"

#include <memory>
#include <ostream>
#include <span>
#include <src/tbprobe.h>
#include <sstream>
#include <utility>

class position_t
{
    class state_t
    {
      public:
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

        [[nodiscard]] square_t ep_square() const { return m_ep_square; }
        [[nodiscard]] piece_t  taken() const { return m_taken; }

        [[nodiscard]] enum_array<color_t, bitboard_t> checkers() const { return m_checkers; }
        [[nodiscard]] enum_array<color_t, bitboard_t> blockers() const { return m_blockers; }
        [[nodiscard]] enum_array<color_t, bitboard_t> check_mask() const { return m_check_mask; }
        [[nodiscard]] castling_rights_t               crs() const { return m_crs; }
        [[nodiscard]] zobrist_t                       hash() const { return m_hash; }
        [[nodiscard]] int                             halfmove_clock() const { return m_halfmove_clock; }

        void crs_remove_rights(const uint8_t lost_mask) { m_crs.remove_right(lost_mask); }

      private:
        square_t                        m_ep_square;
        enum_array<color_t, bitboard_t> m_checkers{};
        enum_array<color_t, bitboard_t> m_blockers{};
        enum_array<color_t, bitboard_t> m_check_mask{};
        castling_rights_t               m_crs;
        zobrist_t                       m_hash;
        piece_t                         m_taken;
        int                             m_halfmove_clock;

        friend class position_t;
    };

  public:
    // board getters
    [[nodiscard]] std::span<const piece_t> pieces() const { return m_pieces; }
    [[nodiscard]] piece_t                  piece_at(const square_t sq) const { return m_pieces.at(sq); }
    [[nodiscard]] piece_type_t             piece_type_at(const square_t sq) const { return piece_at(sq).type(); }
    [[nodiscard]] color_t                  color_at(const square_t sq) const { return piece_at(sq).color(); }
    [[nodiscard]] bool                     is_occupied(const square_t sq) const { return piece_at(sq) != NO_PIECE; }
    template <class... Ts>
    bitboard_t pieces_bb(color_t c, piece_type_t first, const Ts... rest) const;
    template <class... Ts>
    bitboard_t               pieces_bb(piece_type_t first, const Ts... rest) const;
    [[nodiscard]] bitboard_t pieces_bb(color_t c, std::initializer_list<piece_type_t> types) const;
    [[nodiscard]] bitboard_t pieces_bb(std::initializer_list<piece_type_t> types) const;
    [[nodiscard]] square_t ksq(const color_t c) const { return square_t{m_pieces_occupancy.at(c).at(KING).get_lsb()}; }
    [[nodiscard]] int      full_move() const { return m_fullmove; }
    [[nodiscard]] int      ply() const { return m_ply; }

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
    [[nodiscard]] piece_t           taken() const { return state().taken(); }

    // utilities
    [[nodiscard]] bitboard_t attacking_sq_bb(square_t sq) const;
    template <color_t c>
    [[nodiscard]] bool is_legal(move_t move) const;

    template <piece_type_t pt>
    void               update_checkers(color_t c);
    void               update_checkers(color_t c);
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
    bool from_fen(std::string_view fen);

    [[nodiscard]] bool     is_draw() const;
    [[nodiscard]] unsigned wdl_probe() const;
    [[nodiscard]] unsigned dtz_probe() const;

    void push_state()
    {
        m_states.at(m_state_idx + 1) = state_t(m_states.at(m_state_idx));
        m_state_idx++;
    }

    void pop_state() { m_state_idx--; }

    [[nodiscard]] std::string to_string() const;
    friend std::ostream&      operator<<(std::ostream& os, const position_t& pos) { return os << pos.to_string(); }

  private:
    enum_array<square_t, piece_t>                             m_pieces{};
    enum_array<color_t, enum_array<piece_type_t, bitboard_t>> m_pieces_occupancy{};
    enum_array<color_t, bitboard_t>                           m_color_occupancy{};
    bitboard_t                                                m_global_occupancy{};
    color_t                                                   m_color{};
    int                                                       m_ply      = 0;
    int                                                       m_fullmove = 1;

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

inline bitboard_t position_t::pieces_bb(const color_t c, const std::initializer_list<piece_type_t> types) const
{
    bitboard_t result{0};

    for (const piece_type_t pt : types)
        result |= m_pieces_occupancy.at(c).at(pt);

    return result;
}

inline bitboard_t position_t::pieces_bb(const std::initializer_list<piece_type_t> types) const
{
    bitboard_t result{0};

    for (const piece_type_t pt : types)
        result |= pieces_bb(WHITE, {pt}) | pieces_bb(BLACK, {pt});

    return result;
}

inline bitboard_t position_t::attacking_sq_bb(const square_t sq) const
{
    // if a piece attacks a square sq2 from a square sq1 it would attack sq1 from sq2
    // exception made for pawns where this is true only with reversed colors
    return (bb::attacks<QUEEN>(sq, occupancy()) & pieces_bb(QUEEN)) |
           (bb::attacks<ROOK>(sq, occupancy()) & pieces_bb(ROOK)) |
           (bb::attacks<BISHOP>(sq, occupancy()) & pieces_bb(BISHOP)) |
           (bb::attacks<KNIGHT>(sq, occupancy()) & pieces_bb(KNIGHT)) |
           (bb::attacks<PAWN>(sq, occupancy(), BLACK) & pieces_bb(WHITE, PAWN)) |
           (bb::attacks<PAWN>(sq, occupancy(), WHITE) & pieces_bb(BLACK, PAWN)) |
           (bb::attacks<KING>(sq, occupancy()) & pieces_bb(KING));
}

inline bitboard_t position_t::pieces_occupancy(const color_t c, const piece_type_t p) const
{
    return m_pieces_occupancy.at(c).at(p);
}

inline bitboard_t position_t::color_occupancy(const color_t c) const
{
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
        const auto sq         = square_t{enemies.pops_lsb()};
        const auto line       = bb::from_to_excl(sq, ksq(c)) & bb::attacks<pt>(sq);
        const auto on_line    = line & occupancy();
        const auto n_blockers = on_line.popcount();
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
    const auto ksq = static_cast<square_t>(pieces_bb(c, KING).get_lsb());

    m_states.at(m_state_idx).m_blockers.at(c)   = bb::empty();
    m_states.at(m_state_idx).m_check_mask.at(c) = m_states.at(m_state_idx).m_checkers.at(c) =
        attacking_sq_bb(ksq) & color_occupancy(~c);

    update_checkers<BISHOP>(c);
    update_checkers<ROOK>(c);
    update_checkers<QUEEN>(c);
}

inline bool position_t::gives_check(const move_t move) const
{
    assert(piece_at(move.from_sq()).color() == color());
    return (bb::attacks(piece_type_at(move.from_sq()), move.to_sq()) & bb::square(ksq(~color()))) != bb::empty();
}

inline void position_t::set_piece(const piece_t piece, const square_t sq)
{
    assert(!is_occupied(sq));
    const piece_type_t pt = piece.type();
    const color_t      c  = piece.color();
    m_global_occupancy |= m_color_occupancy.at(c) |= m_pieces_occupancy.at(c).at(pt) |= bb::square(sq);
    m_pieces.at(sq) = piece;
}

inline void position_t::set_piece(const piece_type_t piece_type, const color_t color, const square_t sq)
{
    assert(!is_occupied(sq));
    m_global_occupancy |= m_color_occupancy.at(color) |= m_pieces_occupancy.at(color).at(piece_type) |= bb::square(sq);
    m_pieces.at(sq) = piece_t{color, piece_type};
}

inline void position_t::remove_piece(const piece_type_t piece_type, const color_t color, const square_t sq)
{
    const piece_t pc{color, piece_type};
    assert(piece_at(sq) == pc);
    m_pieces_occupancy.at(color).at(piece_type) &= m_color_occupancy.at(color) &= m_global_occupancy &= ~bb::square(sq);
    m_pieces.at(sq) = NO_PIECE;
}

inline void position_t::remove_piece(const piece_t piece, const square_t sq)
{
    const piece_type_t pt = piece.type();
    const color_t      c  = piece.color();
    assert(piece_at(sq) == piece);
    m_pieces_occupancy.at(c).at(pt) &= m_color_occupancy.at(c) &= m_global_occupancy &= ~bb::square(sq);
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

inline bool position_t::from_fen(const std::string_view fen)
{
    m_pieces.fill(NO_PIECE);
    m_color_occupancy.fill(bb::empty());
    m_global_occupancy = bb::empty();
    m_pieces_occupancy.fill(enum_array<piece_type_t, bitboard_t>{});
    m_states.at(0) = state_t();

    std::istringstream iss{std::string(fen)};
    std::string        board_str, color_str, castling_str, ep_str, halfmove_str, fullmove_str;
    iss >> board_str >> color_str >> castling_str >> ep_str >> halfmove_str >> fullmove_str;

    file_t file = FILE_A;
    rank_t rank = RANK_8;

    for (const auto c : board_str)
    {
        if (c == '/')
        {
            --rank;
            file = FILE_A;
        }
        else if (c == '8')
        {
        }
        else if (std::isdigit(c))
        {
            file = file + (c - '0');
        }
        else
        {
            const auto pc = from_char<piece_t>(c);
            if (!pc)
                return false;
            set_piece(pc.value(), square_t{file, rank});

            if (file != FILE_H)
                ++file;
        }
    }

    const auto color = from_string<color_t>(color_str);
    if (!color)
        return false;
    m_color = color.value();

    castling_rights_t& crs = m_states.at(m_state_idx).m_crs;
    crs                    = castling_rights_t::from_string(castling_str);

    const auto ep_square = from_string<square_t>(ep_str);
    if (!ep_square)
        return false;
    m_states.at(m_state_idx).m_ep_square = ep_square.value();

    if (halfmove_str.size() > 3 || fullmove_str.size() > 3)
        return false;

    try
    {
        m_states.at(m_state_idx).m_halfmove_clock = stoi(halfmove_str);
        m_fullmove                                = stoi(fullmove_str);
    }
    catch (const std::invalid_argument& _)
    {
        return false;
    }
    catch (const std::out_of_range& _)
    {
        return false;
    }

    init_state();
    return true;
}

inline std::string position_t::to_string() const
{
    std::ostringstream res{};

    res << "Position:\n";
    res << "Side to move: " << color() << "\n";
    res << "Castling rights: " << m_states.at(m_state_idx).m_crs << "\n";

    res << "En passant square: " << ep_square() << "\n";

    res << "Zobrist hash: 0x" << std::hex << state().hash().value() << std::dec << "\n";

    for (auto rank = RANK_1; rank <= RANK_8; ++rank)
    {
        res << index(rank) + 1 << " ";
        for (auto file = FILE_A; file <= FILE_H; ++file)
        {
            const square_t sq{file, RANK_8 - rank};
            const piece_t  pc = piece_at(sq);
            res << pc << " ";
        }
        res << "\n";
    }

    res << "  a b c d e f g h\n";
    return res.str();
}

template <color_t c>
bool position_t::is_legal(const move_t move) const
{
    constexpr direction_t down    = c == WHITE ? SOUTH : NORTH;
    const bitboard_t      from_bb = bb::square(move.from_sq());
    const bitboard_t      to_bb   = bb::square(move.to_sq());

    if (piece_type_at(move.from_sq()) == KING)
    {
        bitboard_t long_range = checkers(c) & pieces_bb(~c, ROOK, BISHOP, QUEEN);
        while (long_range)
        {
            if (const auto sq = static_cast<square_t>(long_range.pops_lsb());
                bb::are_aligned(sq, move.from_sq(), move.to_sq()) && move.to_sq() != sq)
            {
                return false;
            }
        }
        if ((attacking_sq_bb(move.to_sq()) & color_occupancy(~c)) != bb::empty())
        {
            return false;
        }
    }
    if (move.type_of() == NORMAL || move.type_of() == PROMOTION)
    {
        // check for pins
        if ((bb::square(move.from_sq()) & blockers(c)) != bb::empty())
        {
            if ((bb::square(move.to_sq()) & bb::line(move.from_sq(), ksq(c))) == bb::empty())
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
        bitboard_t ep_occupancy = occupancy();
        ep_occupancy &= ~from_bb;
        ep_occupancy |= to_bb;
        ep_occupancy &= ~bb::shift<down>(to_bb);
        for (const auto piece_type : {BISHOP, ROOK, QUEEN})
        {
            if (pieces_occupancy(~c, piece_type) & bb::attacks(piece_type, ksq(c), ep_occupancy))
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
        state.m_hash.flip_ep(prev_ep.file());
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

        state.m_hash.move_piece(piece_t{us, KING}, k_from, k_to);
        state.m_hash.move_piece(piece_t{us, ROOK}, r_from, r_to);

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
        else if (pt == PAWN && index(to) - index(from) == value_of(2 * up))
        {
            if (((bb::pseudo_attack<PAWN>(to - up, us)) & color_occupancy(~us)) != bb::empty())
            {
                state.m_ep_square = from + up;
                state.m_hash.flip_ep(from.file());
            }
        }
    }
    if (move_type == EN_PASSANT)
    {
        const auto to_ep = to - up;
        state.m_hash.flip_piece(piece_t{~us, PAWN}, to_ep);
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
    state.m_hash.move_piece(piece_t{us, pt}, from, to);
    update_checkers(~us);
}

inline void position_t::do_null_move()
{
    push_state();

    state_t& state = m_states.at(m_state_idx);

    const color_t us = color();

    if (const square_t prev_ep = m_states.at(m_state_idx - 1).ep_square(); prev_ep != NO_SQUARE)
    {
        state.m_hash.flip_ep(prev_ep.file());
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
            assert(taken.color() == ~us);
            set_piece(taken, to);
        }
        remove_piece(move.promotion_type(), us, from);
        set_piece(PAWN, us, from);
    }
    if (move_type == EN_PASSANT)
    {
        // remove two up right / left
        set_piece(PAWN, ~us, to - up);
    }
    if (move_type == NORMAL)
    {
        // capture
        if (taken != NO_PIECE)
        {
            assert(taken.color() == ~us);
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
    int ep_sq = ep_square() == NO_SQUARE ? 0 : value_of(ep_square()) + 1;
    return tb_probe_wdl(color_occupancy(WHITE).value(), color_occupancy(BLACK).value(), pieces_bb(KING).value(),
                        pieces_bb(QUEEN).value(), pieces_bb(ROOK).value(), pieces_bb(BISHOP).value(),
                        pieces_bb(KNIGHT).value(), pieces_bb(PAWN).value(), static_cast<unsigned>(halfmove_clock()),
                        crs().mask(), ep_sq, color() == WHITE);
}

inline unsigned position_t::dtz_probe() const
{
    int ep_sq = ep_square() == NO_SQUARE ? 0 : value_of(ep_square()) + 1;
    return tb_probe_root(color_occupancy(WHITE).value(), color_occupancy(BLACK).value(), pieces_bb(KING).value(),
                         pieces_bb(QUEEN).value(), pieces_bb(ROOK).value(), pieces_bb(BISHOP).value(),
                         pieces_bb(KNIGHT).value(), pieces_bb(PAWN).value(), static_cast<unsigned>(halfmove_clock()),
                         crs().mask(), ep_sq, color() == WHITE, nullptr);
}

#endif
