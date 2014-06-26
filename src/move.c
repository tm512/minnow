/* Copyright (c) 2014 Kyle Davis, All Rights Reserved.

   Permission is hereby granted, free of charge, to any person
   obtaining a copy of this software and associated documentation
   files (the "Software"), to deal in the Software without
   restriction, including without limitation the rights to use,
   copy, modify, merge, publish, distribute, sublicense, and/or sell
   copies of the Software, and to permit persons to whom the
   Software is furnished to do so, subject to the following
   conditions:

   The above copyright notice and this permission notice shall be
   included in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
   OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
   NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
   HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
   OTHER DEALINGS IN THE SOFTWARE. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "int.h"
#include "board.h"
#include "move.h"
#include "values.h"

static const uint32 maxnodes = 8192;

move history [128];
uint8 htop = 0;
uint64 numnodes = 0;

// applies a move to curboard
void move_apply (move *m)
{
	htop ++;
	history [htop] = *m;

	// unset piece on the square we're moving from
	curboard->squares [m->from].piece = NULL;
	curboard->score [curboard->side] -= posvals [curboard->side] [curboard->pieces [m->piece].type] [m->from];

	// set our piece on the new square
	curboard->squares [m->square].piece = &curboard->pieces [m->piece];
	curboard->pieces [m->piece].square = m->square;

	if (m->special == ms_null && (curboard->pieces [m->piece].flags & pf_moved) == 0)
		m->special = ms_firstmove;

	curboard->pieces [m->piece].flags |= pf_moved;

	// kingside castling
	if (m->special == ms_kcast)
	{
		curboard->cast [curboard->side] [1] = 0;
		piece *rook = curboard->rooks [curboard->side] [1];
		curboard->score [curboard->side] -= posvals [curboard->side] [pt_rook] [rook->square];
		curboard->squares [rook->square].piece = NULL;
		curboard->squares [m->square - 1].piece = rook;
		rook->square = m->square - 1;
		curboard->score [curboard->side] += posvals [curboard->side] [pt_rook] [rook->square];
	}

	// queenside castling
	if (m->special == ms_qcast)
	{
		curboard->cast [curboard->side] [0] = 0;
		piece *rook = curboard->rooks [curboard->side] [0];
		curboard->score [curboard->side] -= posvals [curboard->side] [pt_rook] [rook->square];
		curboard->squares [rook->square].piece = NULL;
		curboard->squares [m->square + 1].piece = rook;
		rook->square = m->square + 1;
		curboard->score [curboard->side] += posvals [curboard->side] [pt_rook] [rook->square];
	}

	if (m->special == ms_enpas)
		curboard->enpas = &curboard->pieces [m->piece];
	else
		curboard->enpas = NULL;

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

		curboard->score [curboard->side] += piecevals [curboard->pieces [m->piece].type] - piecevals [pt_pawn];
	}

	curboard->score [curboard->side] += posvals [curboard->side] [curboard->pieces [m->piece].type] [m->square];

	// taking a piece?
	if (m->taken < 32)
	{
		curboard->pieces [m->taken].flags |= pf_taken;
		if (m->special == ms_enpascap)
			curboard->squares [curboard->pieces [m->taken].square].piece = NULL;

		curboard->score [!curboard->side] -= piecevals [curboard->pieces [m->taken].type];
		curboard->score [!curboard->side] -= posvals [!curboard->side] [curboard->pieces [m->taken].type] [curboard->pieces [m->taken].square];
	}

	// switch sides
	curboard->side = !curboard->side;
}

// make a move, setting it as the current root for searching
void move_make (move *m)
{
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
	curboard->score [!curboard->side] -= posvals [!curboard->side] [curboard->pieces [m->piece].type] [m->square];

	// set our piece on the new square
	curboard->squares [m->from].piece = &curboard->pieces [m->piece];
	curboard->pieces [m->piece].square = m->from;

	if (m->special == ms_firstmove || m->special == ms_enpas || m->special == ms_kcast || m->special == ms_qcast)
		curboard->pieces [m->piece].flags &= ~pf_moved;

	// kingside castling
	if (m->special == ms_kcast)
	{
		curboard->cast [!curboard->side] [1] = 1;
		piece *rook = curboard->rooks [!curboard->side] [1];
		curboard->score [!curboard->side] -= posvals [!curboard->side] [pt_rook] [rook->square];
		curboard->squares [rook->square].piece = NULL;
		curboard->squares [m->square + 1].piece = rook;
		rook->square = m->square + 1;
		curboard->score [!curboard->side] += posvals [!curboard->side] [pt_rook] [rook->square];
	}

	// queenside castling
	if (m->special == ms_qcast)
	{
		curboard->cast [!curboard->side] [0] = 1;
		piece *rook = curboard->rooks [!curboard->side] [0];
		curboard->score [!curboard->side] -= posvals [!curboard->side] [pt_rook] [rook->square];
		curboard->squares [rook->square].piece = NULL;
		curboard->squares [m->square - 2].piece = rook;
		rook->square = m->square - 2;
		curboard->score [!curboard->side] += posvals [!curboard->side] [pt_rook] [rook->square];
	}

	if (history [htop].special == ms_enpas)
		curboard->enpas = &curboard->pieces [history [htop].piece];
	else
		curboard->enpas = NULL;

	if (m->special >= ms_qpromo && m->special <= ms_npromo)
	{
		curboard->score [!curboard->side] -= piecevals [curboard->pieces [m->piece].type] - piecevals [pt_pawn];
		curboard->pieces [m->piece].type = pt_pawn;
	}

	curboard->score [!curboard->side] += posvals [!curboard->side] [curboard->pieces [m->piece].type] [m->from];

	// untake a piece
	if (m->taken < 32) 
	{
		curboard->pieces [m->taken].flags &= ~pf_taken;
		curboard->squares [curboard->pieces [m->taken].square].piece = &curboard->pieces [m->taken];
		curboard->score [curboard->side] += piecevals [curboard->pieces [m->taken].type];
		curboard->score [curboard->side] += posvals [curboard->side] [curboard->pieces [m->taken].type] [curboard->pieces [m->taken].square];
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

extern uint8 attackhack;
static int8 pawnforward [2] = { 10, -10 };
static int8 pawn2forward [2] = { 20, -20 };
static int8 pawntake [2][4] = { { 9, 11, -1, 1 }, { -9, -11, 1, -1 } };
movelist *move_pawnmove (uint8 piece, movelist **tail)
{
	uint8 side, notside;
	movelist *ret, *it;
	uint8 sqnum = curboard->pieces [piece].square;
	square *sq = &curboard->squares [sqnum];
	struct piece_s *p = &curboard->pieces [piece];

	ret = it = NULL;

	setside (piece, &side, &notside);

	// forward move possible?
	if (!attackhack && !(sq + pawnforward [side])->padding && !(sq + pawnforward [side])->piece)
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
			it->m.special = ms_qpromo;

			// add the other promotions too
			for (int j = ms_rpromo; j <= ms_npromo; j++)
			{
				it->next = move_newnode (piece, 32, sqnum + pawnforward [side], sqnum);
				it = it->next;
	
				it->m.special = j;
			}
		}
	}

	// see if we can take
	for (int i = 0; i < 4; i++)
	{
		// break if we shouldn't consider en passant moves
		if (i > 1 && !curboard->enpas)
			break;

		if (i > 1 && (sq + pawntake [side] [i])->piece != curboard->enpas)
			continue; // this piece isn't vulnerable to en passant

		if (attackhack || ((sq + pawntake [side] [i])->piece && (sq + pawntake [side] [i])->piece->side == notside
		    && ((sq + pawntake [side] [i])->piece->flags & pf_taken) == 0))
		{
			if (!ret)
				ret = it = move_newnode (piece, 32, sqnum + pawntake [side] [i > 1 ? i - 2 : i], sqnum);
			else
			{
				it->next = move_newnode (piece, 32, sqnum + pawntake [side] [i > 1 ? i - 2 : i], sqnum);
				it = it->next;
			}

			if ((sq + pawntake [side] [i])->piece)
				it->m.taken = (sq + pawntake [side] [i])->piece - curboard->pieces;

			if (i > 1)
				it->m.special = ms_enpascap;

			// moving onto the first rank of the opponent?
			if ((sq + pawn2forward [side])->padding)
			{
				it->m.special = ms_qpromo;

				// add the other promotions too
				for (int j = ms_rpromo; j <= ms_npromo; j++)
				{
					it->next = move_newnode (piece, (sq + pawntake [side] [i])->piece - curboard->pieces,
					                         sqnum + pawntake [side] [i > 1 ? i - 2 : i], sqnum);
					it = it->next;
	
					it->m.special = j;
				}
			}
		}
	}

	*tail = it;
	return ret;
}

static int8 knightmove [8] = { -21, -19, -12, -8, 8, 12, 19, 21 };
movelist *move_knightmove (uint8 piece, movelist **tail)
{
	uint8 side, notside;
	movelist *ret, *it;
	uint8 sqnum = curboard->pieces [piece].square;
	square *sq = &curboard->squares [sqnum];

	ret = it = NULL;

	setside (piece, &side, &notside);

	for (int i = 0; i < 8; i++)
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

	*tail = it;
	return ret;
}

movelist *move_slidermove (uint8 piece, int8 *moves, uint8 nmoves, movelist **tail)
{
	uint8 side, notside;
	movelist *ret, *it;
	uint8 sqnum = curboard->pieces [piece].square;
	square *sq = &curboard->squares [sqnum];
	int8 j;

	ret = it = NULL;

	setside (piece, &side, &notside);

	for (int i = 0; i < nmoves; i++)
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

	*tail = it;
	return ret;
}

static int8 bishopmove [4] = { -11, -9, 9, 11 };
movelist *move_bishopmove (uint8 piece, movelist **tail)
{
	return move_slidermove (piece, bishopmove, 4, tail);
}

static int8 rookmove [4] = { -10, -1, 1, 10 };
movelist *move_rookmove (uint8 piece, movelist **tail)
{
	return move_slidermove (piece, rookmove, 4, tail);
}

static int8 qkmove [8] = { -11, -10, -9, -1, 1, 9, 10, 11 };
movelist *move_queenmove (uint8 piece, movelist **tail)
{
	return move_slidermove (piece, qkmove, 8, tail);
}

movelist *move_kingmove (uint8 piece, movelist **tail)
{
	uint8 side, notside;
	movelist *ret, *it;
	uint8 sqnum = curboard->pieces [piece].square;
	square *sq = &curboard->squares [sqnum];
	struct piece_s *p = &curboard->pieces [piece];

	ret = it = NULL;

	setside (piece, &side, &notside);

	for (int i = 0; i < 8; i++)
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
		    curboard->cast [side] [1] && (curboard->rooks [side] [1]->flags & (pf_moved | pf_taken)) == 0 && 
		    !board_squareattacked (p->square) && !board_squareattacked (p->square + 1) &&
		    !board_squareattacked (p->square + 2))
		{
			it->next = move_newnode (piece, 32, sqnum + 2, sqnum);
			it = it->next;
			it->m.special = ms_kcast;
		}

		// queen side
		if (!curboard->squares [p->square - 1].piece && !curboard->squares [p->square - 2].piece &&
		    !curboard->squares [p->square - 3].piece && curboard->cast [side] [0] && 
		    (curboard->rooks [side] [0]->flags & (pf_moved | pf_taken)) == 0 && !board_squareattacked (p->square) &&
		    !board_squareattacked (p->square - 1) && !board_squareattacked (p->square - 2))
		{
			it->next = move_newnode (piece, 32, sqnum - 2, sqnum);
			it = it->next;
			it->m.special = ms_qcast;
		}

		p->flags &= ~pf_moved;
		curboard->side = !curboard->side;
	}

	*tail = it;
	return ret;
}

movelist *nodes = NULL;

void move_initnodes (void)
{
	nodes = malloc (maxnodes * sizeof (movelist));
	memset (nodes, 0, maxnodes * sizeof (movelist));
	for (int i = 0; i < maxnodes; i++)
		nodes [i].next = &nodes [i + 1];
}

uint64 calls = 0;
movelist *move_newnode (uint8 piece, uint8 taken, uint8 square, uint8 from)
{
	movelist *ret = nodes;
	nodes = nodes->next;

	ret->m.piece = piece;
	ret->m.taken = taken;
	ret->m.square = square;
	ret->m.from = from;
	ret->m.special = 0;

	ret->next = NULL;

	numnodes ++;

	return ret;
}

// recursively clean a list of moves
void move_clearnodes (movelist *m)
{
	if (!m)
		return;

	// don't free this one until we clean out all that it links to
	if (m->next)
		move_clearnodes (m->next);

	m->next = nodes;
	nodes = m;

	numnodes --;
}

static inline movelist *merge (movelist *left, movelist *right)
{
	movelist ret = { .next = NULL }, *it;
	it = &ret;

	while (left && right)
	{
		if (left->m.score > right->m.score)
		{
			it->next = left;
			left = left->next;
		}
		else
		{
			it->next = right;
			right = right->next;
		}

		it = it->next;
	}

	it->next = left ? left : right;

	return ret.next;
}

movelist *move_order (movelist *list)
{
	#ifdef NOSORT
	return list;
	#endif

	movelist *slow, *fast, *half;
	slow = fast = list;

	if (!list || !list->next)
		return list;

	while (fast->next && fast->next->next)
	{
		slow = slow->next;
		fast = fast->next->next;
	}

	half = slow->next;
	slow->next = NULL;

	return merge (move_order (list), move_order (half));
}

// generate child nodes based on the current board
movelist *move_genlist (void)
{
	uint8 side = curboard->side * 16;
	movelist *ret, *m, *it, *tail;

	ret = it = tail = NULL;

	for (int i = side; i < side + 16; i++)
	{
		// taken pieces can't move
		if (curboard->pieces [i].flags & pf_taken || curboard->pieces [i].type == pt_none)
			continue;

		// generate all moves from this piece
		m = curboard->pieces [i].movefunc (i, &tail);

		if (m)
		{
			// add the moves to the list
			if (!ret)
				ret = it = m;
			else
				it->next = m;

			// move it to the end of the list
			it = tail;
		}
	}

	// score moves
	it = ret;
	while (it)
	{
		it->m.score = curboard->victim [it->m.taken] + curboard->attack [it->m.piece];
		it = it->next;
	}

	return ret;
}

void move_print (move *m, char *c)
{
	snprintf (c, 6, "%c%i%c%i", 'a' + board_getfile (m->from), 1 + board_getrank (m->from),
	          'a' + board_getfile (m->square), 1 + board_getrank (m->square));
}

// decode algebraic notation into a move struct
void move_decode (const char *c, move *m)
{
	// a1 is 21, each rank adds 10, and each file adds 1
	m->from = 21 + (c [0] - 'a') + (c [1] - '1') * 10;
	m->square = 21 + (c [2] - 'a') + (c [3] - '1') * 10;

	m->piece = curboard->squares [m->from].piece - curboard->pieces;
	m->taken = curboard->squares [m->square].piece - curboard->pieces;

	// determine move special, if anything
	if (curboard->squares [m->from].piece->type == pt_king)
	{
		// castling kingside?
		if (c [0] == 'e' && c [2] == 'g')
			m->special = ms_kcast;

		if (c [0] == 'e' && c [2] == 'c')
			m->special = ms_qcast;
	}

	if (curboard->squares [m->from].piece->type == pt_pawn)
	{
		if ((c [1] == '2' && c [3] == '4') || (c [1] == '7' && c [3] == '5'))
			m->special = ms_enpas;

		if (c [0] != c [2] && !curboard->squares [m->square].piece)
		{
			m->special = ms_enpascap;
			if (!curboard->side)
				m->taken = curboard->squares [m->square - 10].piece - curboard->pieces;
			else
				m->taken = curboard->squares [m->square + 10].piece - curboard->pieces;
		}

		if (c [4] != ' ' && c [4] != '\0')
		{
			switch (c [4])
			{
				case 'q':
					m->special = ms_qpromo;
				break;
				case 'r':
					m->special = ms_rpromo;
				break;
				case 'b':
					m->special = ms_bpromo;
				break;
				case 'n':
					m->special = ms_npromo;
				break;
			}
		}
	}
}
