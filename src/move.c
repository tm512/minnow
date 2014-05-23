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
	// Add move to history
	if (history)
		m->prev = history;

	history = m;

	// unset piece on the square we're moving from
	curboard->squares [m->from].piece = NULL;

	// set our piece on the new square
	curboard->squares [m->square].piece = &curboard->pieces [m->piece];
	curboard->pieces [m->piece].square = m->square;
//	printf ("moving %i to %i\n", m->piece, m->square);

	curboard->pieces [m->piece].flags |= pf_moved;

	// taking a piece?
	if (m->taken < 32)
		curboard->pieces [m->taken].flags |= pf_taken;

	// switch sides
	curboard->side = !curboard->side;
}

// make a move, setting it as the current root for searching
void move_make (movelist *m)
{
//	printf ("playing as %s\n", curboard->side ? "black" : "white");
	movelist *oldroot = moveroot;
	move_apply (m->m);
	moveroot = m;
	move_clearnodes (oldroot);
}

// undo a move, remove it from history
void move_undo (move *m)
{
	// remove from history
	history = m->prev;
	m->prev = NULL;

	// this is like move_apply, with to and from swapped:

	// unset piece on the square we're moving from
	curboard->squares [m->square].piece = NULL;

	// set our piece on the new square
	curboard->squares [m->from].piece = &curboard->pieces [m->piece];
	curboard->pieces [m->piece].square = m->from;

	if (m->special == ms_firstmove || m->special == ms_enpas)
		curboard->pieces [m->piece].flags &= ~pf_moved;

	// untake a piece
	if (m->taken < 32) 
	{
		curboard->pieces [m->taken].flags &= ~pf_taken;
		curboard->squares [curboard->pieces [m->taken].square].piece = &curboard->pieces [m->taken];
	}

	// switch sides
	curboard->side = !curboard->side;
}

// move generators, returns a movelist or NULL in case there aren't moves for this piece
static inline void setside (uint8 piece, uint8 *side, uint8 *notside)
{
	if (piece < 16) // white
	{
		*side = ps_white;
		*notside = ps_black;
	}
	else
	{
		*side = ps_black;
		*notside = ps_white;
	}
}

static int8 pawnforward [2] = { 10, -10 };
static int8 pawn2forward [2] = { 20, -20 };
static int8 pawntake [2][4] = { { 9, 11, -1, 1 }, { -9, -11, 1, -1 } };
movelist *move_pawnmove (movelist *parent, uint8 piece)
{
	uint8 side, notside;
	movelist *ret, *it;
	uint8 sqnum = curboard->pieces [piece].square;
	square *sq = &curboard->squares [sqnum];
	struct piece_s *p = &curboard->pieces [piece];
	int i;

	ret = it = NULL;

	setside (piece, &side, &notside);

	// forward move possible?
	if (!(sq + pawnforward [side])->padding && !(sq + pawnforward [side])->piece)
	{
		ret = it = move_newnode (parent);

		it->m->piece = piece;
		it->m->square = sqnum + pawnforward [side];
		it->m->from = sqnum;

		// first move, mark this as the move's special, and see if we can move forward 2 ranks
		if ((p->flags & pf_moved) == 0)
		{
			it->m->special = ms_firstmove;
			if (!(sq + pawn2forward [side])->piece)
			{
				it->next = move_newnode (parent);
				it = it->next;

				it->m->piece = piece;
				it->m->square = sqnum + pawn2forward [side];
				it->m->from = sqnum;
				it->m->special = ms_enpas; // this piece is vulnerable to en passant
			}
		}
	}

	// see if we can take
	for (i = 0; i < 4; i++)
	{
		// break if we shouldn't consider en passant moves
		if (i > 1 && (!history || history->special != ms_enpas))
			break;

		if (i > 1 && (sq + pawntake [side] [i])->piece - curboard->pieces != history->piece)
			continue; // this piece isn't vulnerable to en passant

		if ((sq + pawntake [side] [i])->piece && (sq + pawntake [side] [i])->piece->side == notside
		    && ((sq + pawntake [side] [i])->piece->flags & pf_taken) == 0)
		{
			if (!ret)
				ret = it = move_newnode (parent);
			else
			{
				it->next = move_newnode (parent);
				it = it->next;
			}

			it->m->piece = piece;
			it->m->taken = (sq + pawntake [side] [i])->piece - curboard->pieces;
			it->m->square = sqnum + pawntake [side] [i > 1 ? i - 2 : i]; // go to the right square with en passant
			it->m->from = sqnum;

			if (i > 1)
				it->m->special = ms_enpascap;

			if ((p->flags & pf_moved) == 0)
				it->m->special = ms_firstmove;
		}
	}

	return ret;
}

static int8 knightmove [8] = { -21, -19, -12, -8, 8, 12, 19, 21 };
movelist *move_knightmove (movelist *parent, uint8 piece)
{
	uint8 side, notside;
	movelist *ret, *it;
	uint8 sqnum = curboard->pieces [piece].square;
	square *sq = &curboard->squares [sqnum];
	int i;

	ret = it = NULL;

	setside (piece, &side, &notside);

	for (i = 0; i < 8; i++)
	{
		// see if our own piece is here or if this is off the board
		if (((sq + knightmove [i])->piece && (sq + knightmove [i])->piece->side == side)
		    || (sq + knightmove [i])->padding)
			continue;

		if (!ret)
			ret = it = move_newnode (parent);
		else
		{
			it->next = move_newnode (parent);
			it = it->next;
		}

		it->m->piece = piece;
		it->m->square = sqnum + knightmove [i];
		it->m->from = sqnum;

		// taking a piece?
		if ((sq + knightmove [i])->piece && (sq + knightmove [i])->piece->side == notside
		    && ((sq + knightmove [i])->piece->flags & pf_taken) == 0)
			it->m->taken = (sq + knightmove [i])->piece - curboard->pieces;
	}

	return ret;
}

movelist *move_slidermove (movelist *parent, uint8 piece, int8 *moves, uint8 nmoves)
{
	uint8 side, notside;
	movelist *ret, *it;
	uint8 sqnum = curboard->pieces [piece].square;
	square *sq = &curboard->squares [sqnum];
	int i, offs;
	int8 j;

	ret = it = NULL;

	setside (piece, &side, &notside);

	for (i = 0; i < nmoves; i++)
	{
		j = moves [i];
		while (!(sq + j)->padding && (!(sq + j)->piece || (sq + j)->piece->side == notside))
		{
			if (!ret)
				ret = it = move_newnode (parent);
			else
			{
				it->next = move_newnode (parent);
				it = it->next;
			}

			it->m->piece = piece;
			it->m->square = sqnum + j;
			it->m->from = sqnum;

			if ((sq + j)->piece)
			{
				if (((sq + j)->piece->flags & pf_taken) == 0)
					it->m->taken = (sq + j)->piece - curboard->pieces;

				break;
			}

			j += moves [i];
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
	uint8 side, notside;
	movelist *ret, *it;
	uint8 sqnum = curboard->pieces [piece].square;
	square *sq = &curboard->squares [sqnum];
	int i, offs;

	ret = it = NULL;

	setside (piece, &side, &notside);

	for (i = 0; i < 8; i++)
	{
		// see if our own piece is here or if this is off the board
		if (((sq + qkmove [i])->piece && (sq + qkmove [i])->piece->side == side) || (sq + qkmove [i])->padding)
			continue;

		if (!ret)
			ret = it = move_newnode (parent);
		else
		{
			it->next = move_newnode (parent);
			it = it->next;
		}

		it->m->piece = piece;
		it->m->square = sqnum + qkmove [i];
		it->m->from = sqnum;

		// taking a piece?
		if ((sq + qkmove [i])->piece && (sq + qkmove [i])->piece->side == notside
		    && ((sq + qkmove [i])->piece->flags & pf_taken) == 0)
			it->m->taken = (sq + qkmove [i])->piece - curboard->pieces;
	}

	return ret;
}

uint64 numnodes = 0;
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
	ret->m->taken = 32;

	ret->parent = parent;
	ret->next = ret->child = NULL;

	numnodes ++;

	return ret;
}

// recursively clean out all non-chosen nodes
void move_clearnodes (movelist *m)
{
	// don't free this one until we clean out all that it links to
	if (m->next)
	{
		move_clearnodes (m->next);
		m->next = NULL;
	}

	// don't traverse down the new moveroot
	if (m == moveroot)
		return;

	if (m->child)
	{
		move_clearnodes (m->child);
		m->child = NULL;
	}

	free (m->m);
	free (m);

	numnodes --;
}

// generate child nodes based on the current board
void move_genlist (movelist *start)
{
	uint8 side = curboard->side * 16;
	int i;
	movelist *(*movefunc) (movelist *parent, uint8 piece);
	movelist *m, *it;

	it = NULL;

	for (i = side; i < side + 16; i++)
	{
		// taken pieces can't move
		if (curboard->pieces [i].flags & pf_taken)
			continue;

		// set our move function depending on the piece type
		switch (curboard->pieces [i].type)
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
//				printf ("unknown piece type %i\n", curboard->pieces [i] & 0x07);
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

			movelist *p = m;
			while (0 && p)
			{
				printf ("%i: %i, %i to %i, %i\n", p->m->piece, board_getfile (p->m->from),
				        board_getrank (p->m->from), board_getfile (p->m->square), board_getrank (p->m->square));
				p = p->next;
			}

			// move it to the end of the list
			while (it->next)
			{
				it = it->next;
			}
		}
	}
}
