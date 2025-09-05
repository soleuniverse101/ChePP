#ifndef POSITION_H_INCLUDED
#define POSITION_H_INCLUDED
#include "bitboard.h"
#include "types.h"
#include "zobrist.h"

#include <bit>
#include <cmath>
#include <cstring>
#include <memory>
#include <ostream>
#include <src/tbprobe.h>
#include <sstream>
#include <utility>

struct PieceList
{
    using BucketT                             = uint64_t;
    static constexpr int     bits_per_bucket  = 64;
    static constexpr int     bits_per_piece   = std::bit_width(Piece::COUNT_V - 1);
    static constexpr BucketT mask             = (static_cast<BucketT>(1) << bits_per_piece) - 1;
    static constexpr int     pieces_per_buket = bits_per_bucket / bits_per_piece;
    static constexpr int     nb_buckets       = 64 / pieces_per_buket;

    static constexpr BucketT none = []()
    {
        BucketT result = 0;
        for (int i = 0; i < pieces_per_buket; i++)
        {
            result |= NO_PIECE.index() << (i * bits_per_piece);
        }
        return result;
    }();

    using DataT = std::array<BucketT, nb_buckets>;

    static constexpr DataT init = []()
    {
        DataT ret{};
        ret.fill(none);
        return ret;
    }();

    DataT data = init;

    static_assert(none == 0xCCCCCCCCCCCCCCCC && mask == 0b1111);

    constexpr void put(const Piece piece, const Square sq)
    {
        const size_t bucket_idx = sq.index() / pieces_per_buket;
        const size_t idx        = sq.index() % pieces_per_buket;
        const size_t shift      = idx * bits_per_piece;
        data[bucket_idx] &= ~(mask << shift);
        data[bucket_idx] |= piece.index() << shift;
    }

    [[nodiscard]] constexpr Piece piece_at(const Square sq) const
    {
        const size_t bucket_idx = sq.index() / pieces_per_buket;
        const size_t idx        = sq.index() % pieces_per_buket;
        const size_t shift      = idx * bits_per_piece;
        return Piece{(data[bucket_idx] >> shift) & mask};
    }
};

class Position
{
    // we store additional info like piece list to enable lazy evaluation of NNUE later on
    // try to keep it under 192 bytes thought
    class alignas(64) state_t
    {
      public:
        static void init_from_prev(const state_t& prev, state_t& init)
        {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wclass-memaccess"
            std::memcpy(&init, &prev, offsetof(state_t, m_color));
#pragma GCC diagnostic pop

            init.m_color          = ~prev.m_color;
            init.m_halfmove_clock = prev.m_halfmove_clock + 1;
            init.m_fullmove_clock = prev.m_fullmove_clock + (init.m_color == BLACK);
            init.m_ep_square      = NO_SQUARE;
            init.m_taken          = NO_PIECE;
        }

        [[nodiscard]] Square ep_square() const { return m_ep_square; }
        [[nodiscard]] Piece  taken() const { return m_taken; }

        [[nodiscard]] EnumArray<Color, Bitboard> blockers() const { return m_blockers; }
        [[nodiscard]] EnumArray<Color, Bitboard> check_mask() const { return m_check_mask; }
        [[nodiscard]] CastlingRights             crs() const { return m_crs; }
        [[nodiscard]] zobrist_t                  hash() const { return m_hash; }
        [[nodiscard]] int                        halfmove_clock() const { return m_halfmove_clock; }
        [[nodiscard]] Square           ksq(const Color c) const { return m_ksq.at(c); }
        [[nodiscard]] int              full_move() const { return m_fullmove_clock; }
        [[nodiscard]] Color            color() const { return m_color; }
        [[nodiscard]] Bitboard         pieces_occupancy(Color c, PieceType p) const;
        [[nodiscard]] Bitboard         color_occupancy(const Color c) const { return m_color_occupancy.at(c); };
        [[nodiscard]] Bitboard         occupancy() const { return m_global_occupancy; }
        [[nodiscard]] Piece     piece_at(const Square sq) const { return m_pieces.at(sq); }


        void crs_remove_rights(const CastlingRights lost) { m_crs.remove(lost); }

      private:
        // copied
        zobrist_t                      m_hash{};
        //PieceList                    m_pieces{};
        EnumArray<Square, Piece>       m_pieces{};
        EnumArray<Color, Bitboard>     m_color_occupancy{};
        Bitboard                       m_global_occupancy{};
        EnumArray<PieceType, Bitboard> m_pieces_type_occupancy{};
        EnumArray<Color, Square>       m_ksq{};
        CastlingRights                 m_crs{};


        // switch
        Color m_color{};

        // increment
        uint8_t m_halfmove_clock = 0;
        uint8_t m_fullmove_clock = 1;

        // reset
        Square m_ep_square{};
        Piece  m_taken{};

        // recomputed
        EnumArray<Color, Bitboard> m_blockers{};
        EnumArray<Color, Bitboard> m_check_mask{};

        // 8 extra if needed

        friend class Position;
        state_t() = default;
    };

    using PieceListT =  EnumArray<Square, Piece> ;


  public:
    [[nodiscard]] Piece     piece_at(const Square sq) const { return state().m_pieces.at(sq); }
    [[nodiscard]] PieceType piece_type_at(const Square sq) const { return piece_at(sq).type(); }
    [[nodiscard]] Color     color_at(const Square sq) const { return piece_at(sq).color(); }
    [[nodiscard]] bool      is_occupied(const Square sq) const { return piece_at(sq) != NO_PIECE; }
    template <class... Ts>
    Bitboard pieces_bb(Color c, PieceType first, const Ts... rest) const;
    template <class... Ts>
    Bitboard                       pieces_bb(PieceType first, const Ts... rest) const;
    [[nodiscard]] Bitboard         pieces_bb(Color c, std::initializer_list<PieceType> types) const;
    [[nodiscard]] Bitboard         pieces_bb(std::initializer_list<PieceType> types) const;
    [[nodiscard]] Bitboard         attacking_sq_bb(Square sq, Bitboard occ) const;
    [[nodiscard]] bool             is_attacking_sq(Square sq, Color c) const;
    [[nodiscard]] Square           ksq(const Color c) const { return state().m_ksq.at(c); }
    [[nodiscard]] int              full_move() const { return state().m_fullmove_clock; }
    [[nodiscard]] int              ply() const { return m_ply; }
    [[nodiscard]] Color            color() const { return state().m_color; }
    [[nodiscard]] Bitboard         pieces_occupancy(Color c, PieceType p) const;
    [[nodiscard]] Bitboard         color_occupancy(Color c) const;
    [[nodiscard]] Bitboard         occupancy() const { return state().m_global_occupancy; }
    [[nodiscard]] const state_t&   state() const { return m_states.at(m_ply); }
    [[nodiscard]] state_t&         state() { return m_states.at(m_ply); }
    [[nodiscard]] const state_t&   state(const int ply) const { return m_states.at(ply); }
    [[nodiscard]] Bitboard         checkers(const Color c) const { return state().check_mask().at(c) & color_occupancy(~c); }
    [[nodiscard]] Bitboard         blockers(const Color c) const { return state().blockers().at(c); }
    [[nodiscard]] Bitboard         check_mask(Color c) const;
    [[nodiscard]] Square           ep_square() const { return state().ep_square(); }
    [[nodiscard]] hash_t           hash() const { return state().hash().value(); }
    [[nodiscard]] CastlingRights   crs() const { return state().crs(); }
    [[nodiscard]] int              halfmove_clock() const { return state().halfmove_clock(); }
    [[nodiscard]] Piece            taken() const { return state().taken(); }
    [[nodiscard]] const PieceListT& pieces() const { return state().m_pieces; }
    [[nodiscard]] PieceListT&       pieces() { return state().m_pieces; }

    // utilities
    [[nodiscard]] Bitboard attacking_sq_bb(Square sq) const;
    template <Color c>
    [[nodiscard]] bool is_legal(Move move) const;

    template <PieceType pt>
    void update_checkers_and_blockers(Color c);
    void update_checkers_and_blockers(Color c);
    void update_state();

    void set_piece(Piece piece, Square sq);
    void set_piece(PieceType piece_type, Color color, Square sq);
    void remove_piece(Square sq);
    void move_piece(Square from, Square to);

    void do_move(Move move);
    void undo_move();

    void init_state();
    bool from_fen(std::string_view fen);

    [[nodiscard]] bool     is_draw() const;
    [[nodiscard]] unsigned wdl_probe() const;
    [[nodiscard]] unsigned dtz_probe() const;

    [[nodiscard]] std::string to_string() const;
    friend std::ostream&      operator<<(std::ostream& os, const Position& pos) { return os << pos.to_string(); }

    [[nodiscard]] std::optional<Move> from_uci(const std::string_view& uci) const;

  private:
    void push_state()
    {
        state_t::init_from_prev(m_states.at(m_ply), m_states.at(m_ply + 1));
        m_ply++;
    }

    void pop_state() { m_ply--; }

    int                          m_ply = 0;
    std::array<state_t, MAX_PLY> m_states{};
};

template <typename... Ts>
Bitboard Position::pieces_bb(const PieceType first, const Ts... rest) const
{
    if constexpr (sizeof...(rest) == 0)
    {
        return state().m_pieces_type_occupancy.at(first);
    }
    else
    {
        return state().m_pieces_type_occupancy.at(first) | pieces_bb(rest...);
    }
}

template <typename... Ts>
[[nodiscard]] Bitboard Position::pieces_bb(const Color c, const PieceType first, const Ts... rest) const
{
    return pieces_bb(first, rest...) & color_occupancy(c);
}

inline Bitboard Position::pieces_bb(const std::initializer_list<PieceType> types) const
{
    Bitboard result{0};

    for (const PieceType pt : types)
        result |= state().m_pieces_type_occupancy.at(pt);

    return result;
}

inline Bitboard Position::pieces_bb(const Color c, const std::initializer_list<PieceType> types) const
{
    return pieces_bb(types) & color_occupancy(c);
}

inline Bitboard Position::attacking_sq_bb(const Square sq, const Bitboard occ) const
{
    // if a piece attacks a square sq2 from a square sq1 it would attack sq1 from sq2
    // exception made for pawns where this is true only with reversed colors
    return ((attacks<ROOK>(sq, occ) & pieces_bb(ROOK, QUEEN)) | (attacks<BISHOP>(sq, occ) & pieces_bb(BISHOP, QUEEN)) |
            (attacks<KNIGHT>(sq, occ) & pieces_bb(KNIGHT)) | (attacks<PAWN>(sq, occ, BLACK) & pieces_bb(WHITE, PAWN)) |
            (attacks<PAWN>(sq, occ, WHITE) & pieces_bb(BLACK, PAWN)) | (attacks<KING>(sq, occ) & pieces_bb(KING))) &
           occ;
}

inline Bitboard Position::attacking_sq_bb(const Square sq) const
{
    return attacking_sq_bb(sq, occupancy());
}

inline bool Position::is_attacking_sq(const Square sq, const Color c) const
{
    // if a piece attacks a square sq2 from a square sq1 it would attack sq1 from sq2
    // exception made for pawns where this is true only with reversed colors
    return attacks<KNIGHT>(sq, occupancy()) & pieces_bb(c, KNIGHT) ||
           attacks<PAWN>(sq, occupancy(), ~c) & pieces_bb(c, PAWN) ||
           attacks<KING>(sq, occupancy()) & pieces_bb(c, KING) ||
           attacks<BISHOP>(sq, occupancy()) & pieces_bb(c, BISHOP, QUEEN) ||
           attacks<ROOK>(sq, occupancy()) & pieces_bb(c, ROOK, QUEEN);
}

inline Bitboard Position::pieces_occupancy(const Color c, const PieceType p) const
{
    return state().m_pieces_type_occupancy.at(p) & state().m_color_occupancy.at(c);
}

inline Bitboard Position::color_occupancy(const Color c) const
{
    return state().m_color_occupancy.at(c);
}

inline Bitboard Position::check_mask(const Color c) const
{
    return state().check_mask().at(c);
}

template <PieceType pt>
void Position::update_checkers_and_blockers(const Color c)
{
    static_assert(pt == BISHOP || pt == ROOK);
    auto enemies = pieces_bb(~c, pt, QUEEN);
    enemies.for_each_square(
        [&](const Square sq)
        {
            const auto line       = from_to_excl(sq, ksq(c)) & attacks<pt>(sq);
            const auto on_line    = line & occupancy();
            const auto n_blockers = on_line.popcount();
            if (n_blockers == 0)
            {
                // check mask also contains all squares between the long range attacker and the king
                m_states.at(m_ply).m_check_mask.at(c) |= line;
            }
            if (n_blockers == 1)
            {
                // blockers can be of any color
                // blocker from same color are pinned from other color can do a discovered check
                m_states.at(m_ply).m_blockers.at(c) |= on_line;
            }
        });
}

inline void Position::update_checkers_and_blockers(const Color c)
{
    m_states.at(m_ply).m_blockers.at(c)   = bb::empty();
    m_states.at(m_ply).m_check_mask.at(c) = attacking_sq_bb(ksq(c)) & color_occupancy(~c);

    update_checkers_and_blockers<BISHOP>(c);
    update_checkers_and_blockers<ROOK>(c);
}

inline void Position::update_state()
{
    state().m_global_occupancy =
        m_states.at(m_ply).m_color_occupancy.at(WHITE) | m_states.at(m_ply).m_color_occupancy.at(BLACK);
    state().m_ksq = {Square{pieces_bb(WHITE, KING).get_lsb()}, Square{pieces_bb(BLACK, KING).get_lsb()}};

    update_checkers_and_blockers(color());
    update_checkers_and_blockers(~color());
}

inline void Position::set_piece(const Piece piece, const Square sq)
{
    assert(!is_occupied(sq));
    const PieceType pt = piece.type();
    const Color     c  = piece.color();
    state().m_pieces_type_occupancy.at(pt) |= Bitboard(sq);
    state().m_color_occupancy.at(c) |= Bitboard(sq);
    pieces().at(sq) = piece;
}

inline void Position::set_piece(const PieceType piece_type, const Color color, const Square sq)
{
    assert(!is_occupied(sq));

    state().m_pieces_type_occupancy.at(piece_type) |= Bitboard(sq);
    state().m_color_occupancy.at(color) |= Bitboard(sq);
    pieces().at(sq) = Piece{color, piece_type};
}

inline void Position::remove_piece(const Square sq)
{
    const Piece pc = piece_at(sq);
    state().m_pieces_type_occupancy.at(pc.type()) &= ~Bitboard(sq);
    state().m_color_occupancy.at(pc.color()) &= ~Bitboard(sq);
    pieces().at(sq) = NO_PIECE;
}

inline void Position::move_piece(const Square from, const Square to)
{
    const Piece piece = piece_at(from);
    remove_piece(from);
    set_piece(piece, to);
}

inline void Position::init_state()
{
    state().m_hash = zobrist_t{};
    for (auto sq = A1; sq <= H8; sq = ++sq)
    {
        if (piece_at(sq) != NO_PIECE)
            state().m_hash.flip_piece(piece_at(sq), sq);
    }

    if (ep_square() != NO_SQUARE)
        state().m_hash.flip_ep(ep_square().file());

    if (color() == BLACK)
        state().m_hash.flip_color();

    state().m_hash.flip_castling_rights(crs().mask());

    update_state();
}

inline bool Position::from_fen(const std::string_view fen)
{
    m_ply              = 0;
    m_states.at(m_ply) = state_t();

    std::istringstream iss{std::string(fen)};
    std::string        board_str, color_str, castling_str, ep_str, halfmove_str, fullmove_str;
    iss >> board_str >> color_str >> castling_str >> ep_str >> halfmove_str >> fullmove_str;

    File file = FILE_A;
    Rank rank = RANK_8;

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
            const auto pc = Piece::from_string(std::string{c});
            if (!pc)
                return false;
            set_piece(pc.value(), Square{file, rank});

            if (file != FILE_H)
                ++file;
        }
    }

    const auto color = Color::from_string(color_str);
    if (!color)
        return false;
    state().m_color = color.value();

    const auto crs = CastlingRights::from_string(castling_str);
    if (!crs)
        return false;
    m_states.at(m_ply).m_crs = crs.value();

    const auto ep_square = Square::from_string(ep_str);
    if (!ep_square)
        return false;
    m_states.at(m_ply).m_ep_square = ep_square.value();

    if (halfmove_str.size() > 3 || fullmove_str.size() > 3)
        return false;

    try
    {
        m_states.at(m_ply).m_halfmove_clock = stoi(halfmove_str);
        m_states.at(m_ply).m_fullmove_clock = stoi(fullmove_str);
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

inline std::string Position::to_string() const
{
    std::ostringstream res{};

    res << "Position:\n";
    res << "Side to move: " << color() << "\n";
    res << "Castling rights: " << m_states.at(m_ply).m_crs << "\n";

    res << "En passant square: " << ep_square() << "\n";

    res << "Zobrist hash: 0x" << std::hex << state().hash().value() << std::dec << "\n";

    for (auto rank = RANK_1; rank <= RANK_8; ++rank)
    {
        res << (RANK_8 - rank).index() + 1 << " ";
        for (auto file = FILE_A; file <= FILE_H; ++file)
        {
            const Square sq{file, RANK_8 - rank};
            const Piece  pc = piece_at(sq);
            res << pc << " ";
        }
        res << "\n";
    }

    res << "  a b c d e f g h\n";
    return res.str();
}

template <Color c>
bool Position::is_legal(const Move move) const
{
    constexpr Direction down    = c == WHITE ? SOUTH : NORTH;
    const auto          from_bb = Bitboard(move.from_sq());
    const auto          to_bb   = Bitboard(move.to_sq());

    if (ksq(c) == move.from_sq())
    {
        // cannot move along the ray of a long range piece
        Bitboard long_range = checkers(c) & pieces_bb(~c, ROOK, BISHOP, QUEEN);
        while (long_range)
        {
            if (const auto sq = Square{long_range.pop_lsb()};
                are_aligned(sq, move.from_sq(), move.to_sq()) && move.to_sq() != sq)
            {
                return false;
            }
        }
        // cannot move to a square that is attacked
        if (is_attacking_sq(move.to_sq(), ~c))
        {
            return false;
        }
    }
    else if (move.type_of() == NORMAL || move.type_of() == PROMOTION)
    {
        // check for pins
        // if the piece is a blocker
        if (Bitboard(move.from_sq()) & blockers(c))
        {
            // and if it is not moving along the line it is pinned to
            if (!(Bitboard(move.to_sq()) & line(move.from_sq(), ksq(c))))
            {
                // we are breaking the pin, move is illegal
                return false;
            }
        }
    }
    else if (move.type_of() == EN_PASSANT)
    {
        // en passant can create discovered checks so we check for any long range attack
        // we know they have not moved. therefore we only need to update global occupancy to cast rays
        // we check if rays intersect with any long range and if they do that means a there is a check
        if (const Bitboard ep_occupancy = (occupancy() & ~from_bb | to_bb) & ~shift<down>(to_bb);
            pieces_bb(~c, BISHOP, QUEEN) & attacks(BISHOP, ksq(c), ep_occupancy) ||
            pieces_bb(~c, ROOK, QUEEN) & attacks(ROOK, ksq(c), ep_occupancy))
            return false;
    }
    return true;
}

inline void Position::do_move(const Move move)
{
    push_state();

    state().m_hash.flip_color();

    if (const Square prev_ep = m_states.at(m_ply - 1).ep_square(); prev_ep != NO_SQUARE)
    {
        state().m_hash.flip_ep(prev_ep.file());
    }

    if (move == Move::null())
    {
        update_state();
        return;
    }

    state_t& state = m_states.at(m_ply);

    const Square from = move.from_sq();
    const Square to   = move.to_sq();
    Piece        pc   = piece_at(from);
    const Color  us   = pc.color();

    assert(us == ~color());

    const Direction up = us == WHITE ? NORTH : SOUTH;

    const auto lost = state.m_crs.lost_from_move(move);
    state.m_crs.remove(lost);
    state.m_hash.flip_castling_rights(lost.mask());

    if (move.type_of() == CASTLING)
    {
        const auto castling_type = move.castling_type();
        assert(CastlingRights{lost}.has(castling_type));
        auto [k_from, k_to] = castling_type.king_move();
        auto [r_from, r_to] = castling_type.rook_move();

        move_piece(r_from, r_to);
        move_piece(k_from, k_to);

        state.m_hash.move_piece(Piece{us, KING}, k_from, k_to);
        state.m_hash.move_piece(Piece{us, ROOK}, r_from, r_to);

        update_state();
        return;
    }

    if (pc.type() == PAWN || is_occupied(to))
    {
        state.m_halfmove_clock = 0;
    }

    if (move.type_of() == NORMAL || move.type_of() == PROMOTION)
    {

        // capture
        if (is_occupied(to))
        {
            assert(color_at(to) == ~us);

            state.m_taken = piece_at(to);
            state.m_hash.flip_piece(piece_at(to), to);
            remove_piece(to);
        }
        // set new ep square
        else if (pc.type() == PAWN && to.value() - from.value() == 2 * up)
        {
            // only set if ep is actually playable
            if (((pseudo_attack<PAWN>(to - up, us)) & color_occupancy(~us)) != bb::empty())
            {
                state.m_ep_square = from + up;
                state.m_hash.flip_ep(from.file());
            }
        }
    }
    if (move.type_of() == EN_PASSANT)
    {
        const auto to_ep = to - up;
        state.m_hash.flip_piece(Piece{~us, PAWN}, to_ep);
        remove_piece(to_ep);
    }
    if (move.type_of() == PROMOTION)
    {
        remove_piece(from);
        set_piece(move.promotion_type(), us, from);
        pc = Piece{us, move.promotion_type()};
        state.m_hash.promote_piece(us, pc.type(), from);
    }
    move_piece(from, to);
    state.m_hash.move_piece(pc, from, to);

    update_state();
}

inline void Position::undo_move()
{
    pop_state();
}

inline bool Position::is_draw() const
{
    if (halfmove_clock() >= 100)
    {
        return true;
    }
    const hash_t target = hash();
    int          hits   = 1;
    size_t       idx    = m_ply;
    while (idx > 0)
    {
        const state_t& state = m_states.at(--idx);
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

inline unsigned Position::wdl_probe() const
{
    size_t ep_sq = ep_square() == NO_SQUARE ? 0 : ep_square().index() + 1;
    return tb_probe_wdl(color_occupancy(WHITE).value(), color_occupancy(BLACK).value(), pieces_bb(KING).value(),
                        pieces_bb(QUEEN).value(), pieces_bb(ROOK).value(), pieces_bb(BISHOP).value(),
                        pieces_bb(KNIGHT).value(), pieces_bb(PAWN).value(), static_cast<unsigned>(halfmove_clock()),
                        crs().mask(), ep_sq, color() == WHITE);
}

inline unsigned Position::dtz_probe() const
{
    size_t ep_sq = ep_square() == NO_SQUARE ? 0 : ep_square().index() + 1;
    return tb_probe_root(color_occupancy(WHITE).value(), color_occupancy(BLACK).value(), pieces_bb(KING).value(),
                         pieces_bb(QUEEN).value(), pieces_bb(ROOK).value(), pieces_bb(BISHOP).value(),
                         pieces_bb(KNIGHT).value(), pieces_bb(PAWN).value(), static_cast<unsigned>(halfmove_clock()),
                         crs().mask(), ep_sq, color() == WHITE, nullptr);
}

inline std::optional<Move> Position::from_uci(const std::string_view& uci) const
{
    const auto incomplete = Move::from_uci(uci);
    if (!incomplete)
        return std::nullopt;
    if (piece_type_at(incomplete->from_sq()) == PAWN && ep_square() == incomplete->to_sq())
    {
        return Move::make<EN_PASSANT>(incomplete->from_sq(), incomplete->to_sq());
    }
    if (piece_type_at(incomplete->from_sq()) == KING)
    {
        CastlingRights copy = crs();
        while (!copy.empty())
        {
            auto type = CastlingType{bit::get_lsb(copy.mask())};
            if (const auto [k_from, k_to] = type.king_move();
                incomplete->from_sq() == k_from && incomplete->to_sq() == k_to)
            {
                return Move::make<CASTLING>(k_from, k_to, type);
            }
            copy.remove(type);
        }
    }
    return incomplete;
}

#endif
