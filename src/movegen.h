#include "position.h"

typedef enum move_type_t : int8_t
{
    QUIET,
    CAPTURE,
    // will do later
} move_type_t;

template <move_type_t mt>
void gen_pawn_moves(position_t* pos)
{
}
