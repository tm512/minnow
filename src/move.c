#include <stdio.h>

#include "int.h"
#include "board.h"
#include "move.h"

move *history = NULL;
movelist *moveroot = NULL;

// applies a move to curboard, and adds it to history
void move_apply (move *m)
{
	// flag to set on the square
	uint8 side = curboard->flags & bf_side ? sf_bocc : sf_wocc;
	uint8 notside = curboard->flags & bf_side ? sf_wocc : sf_bocc;

	// Add move to history
	if (history)
	{
		m->prev = history;
		history->next = m;
	}

	history = m;

	// unset piece on the square we're moving to and from
	curboard->squares [m->square] &= ~0x1f;
	curboard->squares [m->from] &= ~(0x1f | side);
	curboard->pieces [m->piece] &= ~0x1f8;

	// set our piece on the new square
	curboard->squares [m->square] |= m->piece | side;
	curboard->pieces [m->piece] |= (board_getfile (m->square) << 3) | (board_getrank (m->square) << 6) | pf_moved;

	// taking a piece?
	if (m->taken < 32)
	{
		curboard->pieces [m->taken] |= pf_taken;
		curboard->squares [board_piecesquare (m->taken)] &= ~notside;
	}
}

// undo a move, remove it from history
void move_undo (move *m)
{
	uint8 side = curboard->flags & bf_side ? sf_bocc : sf_wocc;
	uint8 notside = curboard->flags & bf_side ? sf_wocc : sf_bocc;

	// remove from history
	history = m->prev;
	m->prev = m->next = NULL;

	if (history)
		history->next = NULL;

	// this is like move_apply, with to and from swapped:

	// unset piece on the square we're moving to and from
	curboard->squares [m->from] &= ~0x1f;
	curboard->squares [m->square] &= ~(0x1f | side);
	curboard->pieces [m->piece] &= ~0x1f8;

	// set our piece on the new square
	curboard->squares [m->from] |= m->piece | side;
	curboard->pieces [m->piece] |= (board_getfile (m->from) << 3) | (board_getrank (m->from) << 6);

	if (m->special == ms_firstmove)
		curboard->pieces [m->piece] &= ~pf_moved;

	// untake a piece
	if (m->taken < 32) 
	{
		curboard->pieces [m->taken] &= ~pf_taken;
		curboard->squares [board_piecesquare (m->taken)] |= notside;
	}
}
