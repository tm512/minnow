#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "int.h"
#include "board.h"
#include "move.h"

move history [128];
uint8 htop = 0;
movelist *moveroot = NULL;
uint64 numnodes = 0;

// applies a move to curboard
void move_apply (move *m)
{
	htop ++;
	history [htop] = *m;

	// unset piece on the square we're moving from
	curboard->squares [m->from].piece = NULL;

	// set our piece on the new square
	curboard->squares [m->square].piece = &curboard->pieces [m->piece];
	curboard->pieces [m->piece].square = m->square;

	if (m->special == ms_null && (curboard->pieces [m->piece].flags & pf_moved) == 0)
		m->special = ms_firstmove;

	curboard->pieces [m->piece].flags |= pf_moved;

	// kingside castling
	if (m->special == ms_kcast)
	{
		piece *rook = &curboard->pieces [curboard->side * 16 + 13];
		curboard->squares [rook->square].piece = NULL;
		curboard->squares [m->square - 1].piece = rook;
		rook->square = m->square - 1;
	}

	// queenside castling
	if (m->special == ms_qcast)
	{
		piece *rook = &curboard->pieces [curboard->side * 16 + 12];
		curboard->squares [rook->square].piece = NULL;
		curboard->squares [m->square + 1].piece = rook;
		rook->square = m->square + 1;
	}

	if (m->special >= ms_qpromo && m->special <= ms_npromo)
	{
		switch (m->special)
		{
			case ms_qpromo:
				curboard->pieces [m->piece].type = pt_queen;
			break;
			case ms_rpromo:
				curboard->pieces [m->piece].type = pt_rook;
			break;
			case ms_bpromo:
				curboard->pieces [m->piece].type = pt_bishop;
			break;
			case ms_npromo:
				curboard->pieces [m->piece].type = pt_knight;
			break;
		}
	}

	// taking a piece?
	if (m->taken < 32)
	{
		curboard->pieces [m->taken].flags |= pf_taken;
		if (m->special == ms_enpascap)
			curboard->squares [curboard->pieces [m->taken].square].piece = NULL;
	}

	// switch sides
	curboard->side = !curboard->side;
}

// make a move, setting it as the current root for searching
void move_make (move *m)
{
	printf ("%u nodes stored (%fMiB)\n", numnodes,
	        (float)(numnodes * (sizeof (movelist) + sizeof (move))) / 1048576.0f);
	move_apply (m);
	htop = 0;
}

// undo a move
void move_undo (move *m)
{
	htop --;

	// this is like move_apply, with to and from swapped:

	// unset piece on the square we're moving from
	curboard->squares [m->square].piece = NULL;

	// set our piece on the new square
	curboard->squares [m->from].piece = &curboard->pieces [m->piece];
	curboard->pieces [m->piece].square = m->from;

	if (m->special == ms_firstmove || m->special == ms_enpas)
		curboard->pieces [m->piece].flags &= ~pf_moved;

	// kingside castling
	if (m->special == ms_kcast)
	{
		piece *rook = &curboard->pieces [curboard->side * 16 + 13];
		curboard->squares [rook->square].piece = NULL;
		curboard->squares [m->square + 1].piece = rook;
		rook->square = m->square + 1;
	}

	// queenside castling
	if (m->special == ms_qcast)
	{
		piece *rook = &curboard->pieces [curboard->side * 16 + 12];
		curboard->squares [rook->square].piece = NULL;
		curboard->squares [m->square - 2].piece = rook;
		rook->square = m->square - 2;
	}

	if (m->special >= ms_qpromo && m->special <= ms_npromo)
		curboard->pieces [m->piece].type = pt_pawn;

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
movelist *move_pawnmove (uint8 piece)
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
		ret = it = move_newnode (piece, 32, sqnum + pawnforward [side], sqnum);

		// first move, mark this as the move's special, and see if we can move forward 2 ranks
		if ((p->flags & pf_moved) == 0)
		{
			if (!(sq + pawn2forward [side])->piece)
			{
				it->next = move_newnode (piece, 32, sqnum + pawn2forward [side], sqnum);
				it = it->next;

				it->m.special = ms_enpas; // this piece is vulnerable to en passant
			}
		}

		// moving onto the first rank of the opponent?
		if ((sq + pawn2forward [side])->padding)
		{
			int j;
			it->m.special = ms_qpromo;

			// add the other promotions too
			for (j = ms_rpromo; j <= ms_npromo; j++)
			{
				it->next = move_newnode (piece, 32, sqnum + pawnforward [side], sqnum);
				it = it->next;
	
				it->m.special = j;
			}
		}
	}

	// see if we can take
	for (i = 0; i < 4; i++)
	{
		// break if we shouldn't consider en passant moves
		if (i > 1 && (!htop || history [htop].special != ms_enpas))
			break;

		if (i > 1 && (sq + pawntake [side] [i])->piece - curboard->pieces != history [htop].piece)
			continue; // this piece isn't vulnerable to en passant

		if ((sq + pawntake [side] [i])->piece && (sq + pawntake [side] [i])->piece->side == notside
		    && ((sq + pawntake [side] [i])->piece->flags & pf_taken) == 0)
		{
			if (!ret)
				ret = it = move_newnode (piece, (sq + pawntake [side] [i])->piece - curboard->pieces,
				                         sqnum + pawntake [side] [i > 1 ? i - 2 : i], sqnum);
			else
			{
				it->next = move_newnode (piece, (sq + pawntake [side] [i])->piece - curboard->pieces,
				                         sqnum + pawntake [side] [i > 1 ? i - 2 : i], sqnum);
				it = it->next;
			}

			if (i > 1)
				it->m.special = ms_enpascap;

			// moving onto the first rank of the opponent?
			if ((sq + pawn2forward [side])->padding)
			{
				int j;
				it->m.special = ms_qpromo;

				// add the other promotions too
				for (j = ms_rpromo; j <= ms_npromo; j++)
				{
					it->next = move_newnode (piece, (sq + pawntake [side] [i])->piece - curboard->pieces,
					                         sqnum + pawntake [side] [i > 1 ? i - 2 : i], sqnum);
					it = it->next;
	
					it->m.special = j;
				}
			}
		}
	}

	return ret;
}

static int8 knightmove [8] = { -21, -19, -12, -8, 8, 12, 19, 21 };
movelist *move_knightmove (uint8 piece)
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
			ret = it = move_newnode (piece, 32, sqnum + knightmove [i], sqnum);
		else
		{
			it->next = move_newnode (piece, 32, sqnum + knightmove [i], sqnum);
			it = it->next;
		}

		// taking a piece?
		if ((sq + knightmove [i])->piece && (sq + knightmove [i])->piece->side == notside
		    && ((sq + knightmove [i])->piece->flags & pf_taken) == 0)
			it->m.taken = (sq + knightmove [i])->piece - curboard->pieces;
	}

	return ret;
}

movelist *move_slidermove (uint8 piece, int8 *moves, uint8 nmoves)
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
				ret = it = move_newnode (piece, 32, sqnum + j, sqnum);
			else
			{
				it->next = move_newnode (piece, 32, sqnum + j, sqnum);
				it = it->next;
			}

			if ((sq + j)->piece)
			{
				if (((sq + j)->piece->flags & pf_taken) == 0)
					it->m.taken = (sq + j)->piece - curboard->pieces;

				break;
			}

			j += moves [i];
		}
	}

	return ret;
}

static int8 bishopmove [4] = { -11, -9, 9, 11 };
movelist *move_bishopmove (uint8 piece)
{
	return move_slidermove (piece, bishopmove, 4);
}

static int8 rookmove [4] = { -10, -1, 1, 10 };
movelist *move_rookmove (uint8 piece)
{
	return move_slidermove (piece, rookmove, 4);
}

static int8 qkmove [8] = { -11, -10, -9, -1, 1, 9, 10, 11 };
movelist *move_queenmove (uint8 piece)
{
	return move_slidermove (piece, qkmove, 8);
}

movelist *move_kingmove (uint8 piece)
{
	uint8 side, notside;
	movelist *ret, *it;
	uint8 sqnum = curboard->pieces [piece].square;
	square *sq = &curboard->squares [sqnum];
	int i, offs;
	struct piece_s *p = &curboard->pieces [piece];

	ret = it = NULL;

	setside (piece, &side, &notside);

	for (i = 0; i < 8; i++)
	{
		// see if our own piece is here or if this is off the board
		if (((sq + qkmove [i])->piece && (sq + qkmove [i])->piece->side == side) || (sq + qkmove [i])->padding)
			continue;

		if (!ret)
			ret = it = move_newnode (piece, 32, sqnum + qkmove [i], sqnum);
		else
		{
			it->next = move_newnode (piece, 32, sqnum + qkmove [i], sqnum);
			it = it->next;
		}

		// taking a piece?
		if ((sq + qkmove [i])->piece && (sq + qkmove [i])->piece->side == notside
		    && ((sq + qkmove [i])->piece->flags & pf_taken) == 0)
			it->m.taken = (sq + qkmove [i])->piece - curboard->pieces;
	}

	// castling
	if ((p->flags & pf_moved) == 0)
	{
		curboard->side = !curboard->side;
		p->flags |= pf_moved;

		// king side
		// make sure squares are empty, and that the king isn't in check, passing through check, or going into check
		if (!curboard->squares [p->square + 1].piece && !curboard->squares [p->square + 2].piece &&
		    (curboard->pieces [13].flags & pf_moved) == 0 && !board_squareattacked (p->square) &&
		    !board_squareattacked (p->square + 1) && !board_squareattacked (p->square + 2))
		{
			it->next = move_newnode (piece, 32, sqnum + 2, sqnum);
			it->m.special = ms_kcast;
		}

		// queen side
		if (!curboard->squares [p->square - 1].piece && !curboard->squares [p->square - 2].piece &&
		    !curboard->squares [p->square - 3].piece && (curboard->pieces [12].flags & pf_moved) == 0 && 
		    !board_squareattacked (p->square) && !board_squareattacked (p->square - 1) &&
		    !board_squareattacked (p->square - 2))
		{
			it->next = move_newnode (piece, 32, sqnum - 2, sqnum);
			it->m.special = ms_qcast;
		}

		p->flags &= ~pf_moved;
		curboard->side = !curboard->side;
	}

	return ret;
}

movelist *move_newnode (uint8 piece, uint8 taken, uint8 square, uint8 from)
{
	movelist *ret = malloc (sizeof (movelist));

	if (!ret)
		return NULL;

	ret->m.piece = piece;
	ret->m.taken = taken;
	ret->m.square = square;
	ret->m.from = from;
	ret->m.special = 0;

	ret->next = NULL;

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

	free (m);

	numnodes --;
}

// generate child nodes based on the current board
movelist *move_genlist (void)
{
	uint8 side = curboard->side * 16;
	int i;
	movelist *(*movefunc) (uint8 piece);
	movelist *ret, *m, *it;

	ret = it = NULL;

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
			break;
		}

		// generate all moves from this piece
		if (movefunc)
			m = movefunc (i);
		else	
			continue;

		if (m)
		{
			// add the moves to the list
			if (!ret)
				ret = it = m;
			else
				it->next = m;

			// move it to the end of the list
			while (it->next)
			{
				it = it->next;
			}
		}
	}

	return ret;
}
