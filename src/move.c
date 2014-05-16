#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

	// switch sides
	curboard->flags ^= bf_side;
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

	// switch sides
	curboard->flags ^= bf_side;
}

// move generators, returns a movelist or NULL in case there aren't moves for this piece
static inline void setside (uint8 piece, uint8 *side, uint8 *occ, uint8 *notocc)
{
	if (piece < 16) // white
	{
		*side = 0;
		*occ = sf_wocc;
		*notocc = sf_bocc;
	}
	else
	{
		*side = 1;
		*occ = sf_bocc;
		*notocc = sf_wocc;
	}
}

static int8 pawnforward [2] = { 10, -10 };
static int8 pawn2forward [2] = { 20, -20 };
static int8 pawntake [2][2] = { { 9, 11 }, { -9, -11 } };
movelist *move_pawnmove (movelist *parent, uint8 piece)
{
	uint8 side, occ, notocc;
	movelist *ret, *it;
	uint8 square = board_piecesquare (piece);
	int i;

	ret = it = NULL;

	setside (piece, &side, &occ, &notocc);

	// forward move possible?
	if ((curboard->squares [square + pawnforward [side]] & (occ | notocc)) == 0)
	{
		ret = it = move_newnode (parent);

		it->m->piece = piece;
		it->m->taken = 32;
		it->m->square = square + pawnforward [side];
		it->m->from = square;

		// first move, mark this as the move's special, and see if we can move forward 2 ranks
		if ((curboard->pieces [piece] & pf_moved) == 0)
		{
			it->m->special = ms_firstmove;
			if ((curboard->squares [square + pawn2forward [side]] & (occ | notocc)) == 0)
			{
				it->next = move_newnode (parent);
				it = it->next;

				it->m->piece = piece;
				it->m->taken = 32;
				it->m->square = square + pawn2forward [side];
				it->m->from = square;
				it->m->special = ms_firstmove;
			}
		}
	}

	// see if we can take
	for (i = 0; i < 2; i++)
	{
		if (curboard->squares [square + pawntake [side] [i]] & notocc)
		{
			if (!ret)
				ret = it = move_newnode (parent);
			else
			{
				it->next = move_newnode (parent);
				it = it->next;
			}

			it->m->piece = piece;
			it->m->taken = curboard->squares [square + pawntake [side] [i]] & 0x1f;
			it->m->square = curboard->squares [square + pawntake [side] [i]];
			it->m->from = square;

			if (curboard->pieces [piece] & pf_moved == 0)
				it->m->special = ms_firstmove;
		}
	}

	//printf ("returning %x\n", ret);
	return ret;
}

static int8 knightmove [8] = { -21, -19, -12, -8, 8, 12, 19, 21 };
movelist *move_knightmove (movelist *parent, uint8 piece)
{
	uint8 side, occ, notocc;
	movelist *ret, *it;
	uint8 square = board_piecesquare (piece);
	int i;

	ret = it = NULL;

	setside (piece, &side, &occ, &notocc);

	for (i = 0; i < 8; i++)
	{
		// see if our own piece is here or if this is off the board
		if (curboard->squares [square + knightmove [i]] & (occ | sf_padding))
			continue;

		if (!ret)
			ret = it = move_newnode (parent);
		else
		{
			it->next = move_newnode (parent);
			it = it->next;
		}

		it->m->piece = piece;
		it->m->square = square + knightmove [i];
		it->m->from = square;

		// taking a piece?
		if (curboard->squares [square + knightmove [i]] & notocc)
			it->m->taken = curboard->squares [square + knightmove [i]] & 0x1f;
	}

	return ret;
}

movelist *move_slidermove (movelist *parent, uint8 piece, int8 *moves, uint8 nmoves)
{
	uint8 side, occ, notocc;
	movelist *ret, *it;
	uint8 square = board_piecesquare (piece);
	int i, j;

	ret = it = NULL;

	setside (piece, &side, &occ, &notocc);

	for (i = 0; i < nmoves; i++)
	{
		j = 1;
		// TODO: maybe precalculate the landing square to avoid all this multiplication
		while ((curboard->squares [square + moves [i] * j] & (occ | sf_padding)) == 0)
		{
			if (!ret)
				ret = it = move_newnode (parent);
			else
			{
				it->next = move_newnode (parent);
				it = it->next;
			}

			it->m->piece = piece;
			it->m->square = square + moves [i] * j;
			it->m->from = square;

			if (curboard->squares [square + moves [i] * j] & notocc)
				it->m->taken = curboard->squares [square + moves [i] * j] & 0x1f;

			j ++;
		}
	}

	return ret;
}

static int8 bishopmove [4] = { -11, -9, 9, 11 };
movelist *move_bishopmove (movelist *parent, uint8 piece)
{
	return move_slidermove (parent, piece, bishopmove, 4);
}

static int8 rookmove [4] = { -10, -1, 1, 10 };
movelist *move_rookmove (movelist *parent, uint8 piece)
{
	return move_slidermove (parent, piece, rookmove, 4);
}

static int8 qkmove [8] = { -11, -10, -9, -1, 1, 9, 10, 11 };
movelist *move_queenmove (movelist *parent, uint8 piece)
{
	return move_slidermove (parent, piece, qkmove, 8);
}

// TODO: castling
movelist *move_kingmove (movelist *parent, uint8 piece)
{
	uint8 side, occ, notocc;
	movelist *ret, *it;
	uint8 square = board_piecesquare (piece);
	int i;

	ret = it = NULL;

	setside (piece, &side, &occ, &notocc);

	for (i = 0; i < 8; i++)
	{
		// see if our own piece is here or if this is off the board
		if (curboard->squares [square + qkmove [i]] & (occ | sf_padding))
			continue;

		if (!ret)
			ret = it = move_newnode (parent);
		else
		{
			it->next = move_newnode (parent);
			it = it->next;
		}

		it->m->piece = piece;
		it->m->square = square + qkmove [i];
		it->m->from = square;

		// taking a piece?
		if (curboard->squares [square + qkmove [i]] & notocc)
			it->m->taken = curboard->squares [square + qkmove [i]] & 0x1f;
	}

	return ret;
}

movelist *move_newnode (movelist *parent)
{
	movelist *ret = malloc (sizeof (movelist));

	if (!ret)
		return NULL;

	ret->m = malloc (sizeof (move));

	if (!ret->m)
	{
		free (ret);
		return NULL;
	}

	memset (ret->m, 0, sizeof (move));

	ret->score = 0;

	ret->parent = parent;
	ret->next = ret->child = NULL;

	return ret;
}

// generate child nodes based on the current board
void move_genlist (movelist *start)
{
	uint8 side = !!(curboard->flags & bf_side);
	uint8 occ = curboard->flags & bf_side ? sf_bocc : sf_wocc;
	uint8 notocc = curboard->flags & bf_side ? sf_wocc : sf_bocc;
	int i;
	movelist *(*movefunc) (movelist *parent, uint8 piece);
	movelist *m, *it;

	it = NULL;

	for (i = 0; i < 16; i++)
	{
		// taken pieces can't move
		if (curboard->pieces [i] & pf_taken)
			continue;

		// set our move function depending on the piece type
		switch (curboard->pieces [i] & 0x07)
		{
			case pt_pawn:
				movefunc = move_pawnmove;
			break;
			case pt_knight:
				movefunc = move_knightmove;
			break;
			case pt_bishop:
				movefunc = move_bishopmove;
			break;
			case pt_rook:
				movefunc = move_rookmove;
			break;
			case pt_queen:
				movefunc = move_queenmove;
			break;
			case pt_king:
				movefunc = move_kingmove;
			break;
			default:
				movefunc = NULL;
			//	printf ("unknown piece type %i\n", curboard->pieces [i] & 0x07);
			break;
		}

		// generate all moves from this piece
		if (movefunc)
			m = movefunc (start, i);
		else	
			continue;

		if (m)
		{
			// add the moves to the list
			if (!start->child)
				start->child = it = m;
			else
				it->next = m;

			/*
			movelist *p = m;
			while (p)
			{
				printf ("%i: %i, %i to %i, %i\n", p->m->piece, board_getfile (p->m->from),
				        board_getrank (p->m->from), board_getfile (p->m->square), board_getrank (p->m->square));
				p = p->next;
			}
			*/

			// move it to the end of the list
			while (it->next)
			{
				it = it->next;
			}
		}
	}
}
