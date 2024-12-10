/* Copyright (c) 2014-2022 Kyle Davis, All Rights Reserved.

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
#include <assert.h>

#include "int.h"
#include "board.h"
#include "move.h"
#include "values.h"
#include "hash.h"

#define NOSORT 0

const uint32 maxnodes = 8192;

move history [1024];
uint64 histkeys [1024];
uint16 htop = 0;
uint64 numnodes = 0;

// applies a move to curboard
void move_apply (move *m)
{
	history [htop] = *m;

	// unset piece on the square we're moving from
	curboard->squares [m->from].piece = NULL;
	curboard->posscore [curboard->side] -= posvals [curboard->side] [curboard->pieces [m->piece].type] [m->from];
	poskey ^= keytable [hash_pieceidx (m->piece) + m->from];

	// set our piece on the new square
	curboard->squares [m->square].piece = &curboard->pieces [m->piece];
	curboard->pieces [m->piece].square = m->square;

	if (m->special == ms_null && (curboard->pieces [m->piece].flags & pf_moved) == 0)
		m->special = ms_firstmove;

	curboard->pieces [m->piece].flags |= pf_moved;

	// kingside castling
	if (m->special == ms_kcast)
	{
		piece *rook = curboard->rooks [curboard->side] [1];
		curboard->posscore [curboard->side] -= posvals [curboard->side] [pt_rook] [rook->square];
		poskey ^= keytable [hash_pieceidx (rook - curboard->pieces) + rook->square];
		curboard->squares [rook->square].piece = NULL;
		curboard->squares [m->square - 1].piece = rook;
		rook->square = m->square - 1;
		rook->flags |= pf_moved;
		curboard->posscore [curboard->side] += posvals [curboard->side] [pt_rook] [rook->square];
		poskey ^= keytable [hash_pieceidx (rook - curboard->pieces) + rook->square];
	}

	// queenside castling
	if (m->special == ms_qcast)
	{
		piece *rook = curboard->rooks [curboard->side] [0];
		curboard->posscore [curboard->side] -= posvals [curboard->side] [pt_rook] [rook->square];
		poskey ^= keytable [hash_pieceidx (rook - curboard->pieces) + rook->square];
		curboard->squares [rook->square].piece = NULL;
		curboard->squares [m->square + 1].piece = rook;
		rook->square = m->square + 1;
		rook->flags |= pf_moved;
		curboard->posscore [curboard->side] += posvals [curboard->side] [pt_rook] [rook->square];
		poskey ^= keytable [hash_pieceidx (rook - curboard->pieces) + rook->square];
	}

	// remove castling permissions for king/rook moves
	if (curboard->pieces [m->piece].type == pt_king)
	{
		if (curboard->cast [curboard->side] [0])
		{
			curboard->cast [curboard->side] [0] = 0;
			poskey ^= castkeys [curboard->side * 2];
		}

		if (curboard->cast [curboard->side] [1])
		{
			curboard->cast [curboard->side] [1] = 0;
			poskey ^= castkeys [curboard->side * 2 + 1];
		}
	}

	if (&curboard->pieces [m->piece] == curboard->rooks [curboard->side] [0] && curboard->cast [curboard->side] [0])
	{
		curboard->cast [curboard->side] [0] = 0;
		poskey ^= castkeys [curboard->side * 2];
	}

	if (&curboard->pieces [m->piece] == curboard->rooks [curboard->side] [1] && curboard->cast [curboard->side] [1])
	{
		curboard->cast [curboard->side] [1] = 0;
		poskey ^= castkeys [curboard->side * 2 + 1];
	}

	// unset old en passant
	if (curboard->enpas)
		poskey ^= epkeys [board_getfile (curboard->enpas->square)];

	if (m->special == ms_enpas)
	{
		curboard->enpas = &curboard->pieces [m->piece];
		poskey ^= epkeys [board_getfile (curboard->enpas->square)];
	}
	else
		curboard->enpas = NULL;

	if (m->special >= ms_qpromo && m->special <= ms_npromo)
	{
		switch (m->special)
		{
			case ms_qpromo:
				curboard->pieces [m->piece].type = pt_queen;
				curboard->pieces [m->piece].movefunc = move_queenmove;
			break;
			case ms_rpromo:
				curboard->pieces [m->piece].type = pt_rook;
				curboard->pieces [m->piece].movefunc = move_rookmove;
			break;
			case ms_bpromo:
				curboard->pieces [m->piece].type = pt_bishop;
				curboard->pieces [m->piece].movefunc = move_bishopmove;
			break;
			case ms_npromo:
				curboard->pieces [m->piece].type = pt_knight;
				curboard->pieces [m->piece].movefunc = move_knightmove;
			break;
		}

		curboard->matscore [curboard->side] += piecevals [curboard->pieces [m->piece].type] - piecevals [pt_pawn];
		curboard->piececount [curboard->side] [pt_pawn] --;
		curboard->piececount [curboard->side] [curboard->pieces [m->piece].type] ++;
		poskey ^= promokeys [m->piece] [m->special - 6];
	}

	curboard->posscore [curboard->side] += posvals [curboard->side] [curboard->pieces [m->piece].type] [m->square];
	poskey ^= keytable [hash_pieceidx (m->piece) + m->square];

	// taking a piece?
	if (m->taken < 32)
	{
		curboard->pieces [m->taken].flags |= pf_taken;
		if (m->special == ms_enpascap)
			curboard->squares [curboard->pieces [m->taken].square].piece = NULL;

		curboard->matscore [!curboard->side] -= piecevals [curboard->pieces [m->taken].type];
		curboard->posscore [!curboard->side] -= posvals [!curboard->side] [curboard->pieces [m->taken].type] [curboard->pieces [m->taken].square];
		curboard->piececount [!curboard->side] [curboard->pieces [m->taken].type] --;
		poskey ^= keytable [hash_pieceidx (m->taken) + curboard->pieces [m->taken].square];

		if (&curboard->pieces [m->taken] == curboard->rooks [!curboard->side] [0] && curboard->cast [!curboard->side] [0])
		{
			curboard->cast [!curboard->side] [0] = 0;
			poskey ^= castkeys [!curboard->side * 2];
		}
	
		if (&curboard->pieces [m->taken] == curboard->rooks [!curboard->side] [1] && curboard->cast [!curboard->side] [1])
		{
			curboard->cast [!curboard->side] [1] = 0;
			poskey ^= castkeys [!curboard->side * 2 + 1];
		}

		// every time we immediately recapture a piece, it counts as a trade.
		if (htop > 0 && m->taken == history [htop - 1].piece && history [htop - 1].taken != 32)
			curboard->trades ++;
	}

	// switch sides
	poskey ^= sidekey;
	curboard->side = !curboard->side;

	histkeys [htop] = poskey;
	htop ++;
}

// undo a move
void move_undo (move *m)
{
	htop --;

	// unset piece on the square we're moving from
	curboard->squares [m->square].piece = NULL;
	curboard->posscore [!curboard->side] -= posvals [!curboard->side] [curboard->pieces [m->piece].type] [m->square];
	poskey ^= keytable [hash_pieceidx (m->piece) + m->square];

	// set our piece on the new square
	curboard->squares [m->from].piece = &curboard->pieces [m->piece];
	curboard->pieces [m->piece].square = m->from;

	if (m->special == ms_firstmove || m->special == ms_enpas || m->special == ms_kcast || m->special == ms_qcast)
		curboard->pieces [m->piece].flags &= ~pf_moved;

	// kingside castling
	if (m->special == ms_kcast)
	{
		piece *rook = curboard->rooks [!curboard->side] [1];
		curboard->posscore [!curboard->side] -= posvals [!curboard->side] [pt_rook] [rook->square];
		poskey ^= keytable [hash_pieceidx (rook - curboard->pieces) + rook->square];
		curboard->squares [rook->square].piece = NULL;
		curboard->squares [m->square + 1].piece = rook;
		rook->square = m->square + 1;
		rook->flags &= ~pf_moved;
		curboard->posscore [!curboard->side] += posvals [!curboard->side] [pt_rook] [rook->square];
		poskey ^= keytable [hash_pieceidx (rook - curboard->pieces) + rook->square];
	}

	// queenside castling
	if (m->special == ms_qcast)
	{
		piece *rook = curboard->rooks [!curboard->side] [0];
		curboard->posscore [!curboard->side] -= posvals [!curboard->side] [pt_rook] [rook->square];
		poskey ^= keytable [hash_pieceidx (rook - curboard->pieces) + rook->square];
		curboard->squares [rook->square].piece = NULL;
		curboard->squares [m->square - 2].piece = rook;
		rook->square = m->square - 2;
		rook->flags &= ~pf_moved;
		curboard->posscore [!curboard->side] += posvals [!curboard->side] [pt_rook] [rook->square];
		poskey ^= keytable [hash_pieceidx (rook - curboard->pieces) + rook->square];
	}

	// restore castling permissions for king/rook moves
	if (curboard->pieces [m->piece].type == pt_king && (m->special == ms_firstmove || m->special == ms_kcast || m->special == ms_qcast))
	{
		if ((curboard->rooks [!curboard->side] [0]->flags & (pf_moved | pf_taken)) == 0)
		{
			curboard->cast [!curboard->side] [0] = 1;
			poskey ^= castkeys [!curboard->side * 2];
		}

		if ((curboard->rooks [!curboard->side] [1]->flags & (pf_moved | pf_taken)) == 0)
		{
			curboard->cast [!curboard->side] [1] = 1;
			poskey ^= castkeys [!curboard->side * 2 + 1];
		}
	}

	if ((curboard->kings [!curboard->side]->flags & pf_moved) == 0)
	{
		if (&curboard->pieces [m->piece] == curboard->rooks [!curboard->side] [0] && (curboard->pieces [m->piece].flags & pf_moved) == 0)
		{
			curboard->cast [!curboard->side] [0] = 1;
			poskey ^= castkeys [!curboard->side * 2];
		}

		if (&curboard->pieces [m->piece] == curboard->rooks [!curboard->side] [1] && (curboard->pieces [m->piece].flags & pf_moved) == 0)
		{
			curboard->cast [!curboard->side] [1] = 1;
			poskey ^= castkeys [!curboard->side * 2 + 1];
		}
	}

	// unset old en passant
	if (curboard->enpas)
		poskey ^= epkeys [board_getfile (curboard->enpas->square)];

	if (htop > 0 && history [htop - 1].special == ms_enpas)
	{
		curboard->enpas = &curboard->pieces [history [htop - 1].piece];
		poskey ^= epkeys [board_getfile (curboard->enpas->square)];
	}
	else
		curboard->enpas = NULL;

	if (m->special >= ms_qpromo && m->special <= ms_npromo)
	{
		curboard->matscore [!curboard->side] -= piecevals [curboard->pieces [m->piece].type] - piecevals [pt_pawn];
		curboard->piececount [!curboard->side] [curboard->pieces [m->piece].type] --;
		curboard->piececount [!curboard->side] [pt_pawn] ++;

		curboard->pieces [m->piece].type = pt_pawn;
		curboard->pieces [m->piece].movefunc = move_pawnmove;
		poskey ^= promokeys [m->piece] [m->special - 6];
	}

	curboard->posscore [!curboard->side] += posvals [!curboard->side] [curboard->pieces [m->piece].type] [m->from];
	poskey ^= keytable [hash_pieceidx (m->piece) + m->from];

	// untake a piece
	if (m->taken < 32) 
	{
		curboard->pieces [m->taken].flags &= ~pf_taken;
		curboard->squares [curboard->pieces [m->taken].square].piece = &curboard->pieces [m->taken];
		curboard->matscore [curboard->side] += piecevals [curboard->pieces [m->taken].type];
		curboard->posscore [curboard->side] += posvals [curboard->side] [curboard->pieces [m->taken].type] [curboard->pieces [m->taken].square];
		curboard->piececount [curboard->side] [curboard->pieces [m->taken].type] ++;
		poskey ^= keytable [hash_pieceidx (m->taken) + curboard->pieces [m->taken].square];

		if ((curboard->kings [curboard->side]->flags & pf_moved) == 0)
		{
			if (&curboard->pieces [m->taken] == curboard->rooks [curboard->side] [0] && (curboard->pieces [m->taken].flags & pf_moved) == 0)
			{
				curboard->cast [curboard->side] [0] = 1;
				poskey ^= castkeys [curboard->side * 2];
			}
		
			if (&curboard->pieces [m->taken] == curboard->rooks [curboard->side] [1] && (curboard->pieces [m->taken].flags & pf_moved) == 0)
			{
				curboard->cast [curboard->side] [1] = 1;
				poskey ^= castkeys [curboard->side * 2 + 1];
			}
		}

		if (htop > 0 && m->taken == history [htop - 1].piece && history [htop - 1].taken != 32)
			curboard->trades --;
	}

	// unset ms_firstmove, since this move may be used in the future for an already moved piece
	if (m->special == ms_firstmove)
		m->special = ms_null;

	// switch sides
	poskey ^= sidekey;
	curboard->side = !curboard->side;
}

void move_applynull (void)
{
	history [htop].piece = 0;
	history [htop].taken = 32;
	history [htop].square = 0;
	history [htop].from = 0;
	history [htop].special = 0;

	// unset old en passant
	if (curboard->enpas)
		poskey ^= epkeys [board_getfile (curboard->enpas->square)];

	curboard->enpas = NULL;

	poskey ^= sidekey;
	histkeys [htop] = poskey;
	htop ++;
}

void move_undonull (void)
{
	htop --;

	if (htop > 0 && history [htop - 1].special == ms_enpas)
	{
		curboard->enpas = &curboard->pieces [history [htop - 1].piece];
		poskey ^= epkeys [board_getfile (curboard->enpas->square)];
	}
	else
		curboard->enpas = NULL;

	poskey ^= sidekey;
}

// called from board_initialize after the position is set
void move_inithist (void)
{
	// add the initial position's key for repetition checks
	histkeys [htop] = poskey;

	// if the initial position has EP set, add a dummy EP move to the history so move_undo resets it properly
	// a bit of a hack, but easier than the alternative of using a copy-make strategy for tracking EP
	if (curboard->enpas)
	{
		history [htop].piece = curboard->enpas - curboard->pieces;
		history [htop].taken = 32;
		history [htop].square = curboard->enpas->square;
		history [htop].from = curboard->enpas->square - 20; // down 2 ranks
		history [htop].special = ms_enpas;
	}
	else
	{
		history [htop].piece = 32;
		history [htop].special = ms_null;
	}

	htop ++;
}

void move_printhist (void)
{
	char c [6];
	char *special, *moveside, *takeside;
	for (int i = htop - 1; i >= 0; i--)
	{
		if (history [i].piece == 32)
		{
			printf ("%i: invalid move\n", i);
			continue;
		}

		move_print (&history [i], c);
		moveside = history [i].piece < 16 ? "white" : "black";

		printf ("%i: %s (moved: %s %s, ", i, c, moveside, board_piecetype (&curboard->pieces [history [i].piece]));

		if (history [i].taken < 32)
		{
			takeside = history [i].taken < 16 ? "white" : "black";
			printf ("taken: %s %s, ", takeside, board_piecetype (&curboard->pieces [history [i].taken]));
		}
		else
			printf ("taken: N/A, ");

		switch (history [i].special)
		{
			case ms_null: special = "null"; break;
			case ms_firstmove: special = "firstmove"; break;
			case ms_enpas: special = "enpas"; break;
			case ms_enpascap: special = "enpascap"; break;
			case ms_kcast: special = "kcast"; break;
			case ms_qcast: special = "qcast"; break;
			case ms_qpromo: special = "qpromo"; break;
			case ms_rpromo: special = "rpromo"; break;
			case ms_npromo: special = "npromo"; break;
			case ms_bpromo: special = "bpromo"; break;
		}

		printf ("special: %s)\n", special);
	}
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
			{
				it->m.taken = (sq + pawntake [side] [i])->piece - curboard->pieces;
				it->m.score += curboard->victim [it->m.taken];
			}

			if (i > 1)
				it->m.special = ms_enpascap;

			// moving onto the first rank of the opponent?
			if (!attackhack && (sq + pawn2forward [side])->padding)
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
		{
			it->m.taken = (sq + knightmove [i])->piece - curboard->pieces;
			it->m.score += curboard->victim [it->m.taken];
		}
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
				{
					it->m.taken = (sq + j)->piece - curboard->pieces;
					it->m.score += curboard->victim [it->m.taken];
				}

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
		{
			it->m.taken = (sq + qkmove [i])->piece - curboard->pieces;
			it->m.score += curboard->victim [it->m.taken];
		}
	}

	// castling
	if ((p->flags & pf_moved) == 0)
	{
		curboard->side = !curboard->side;
		p->flags |= pf_moved;

		// king side
		// make sure squares are empty, and that the king isn't in check, passing through check, or going into check
		if (!curboard->squares [p->square + 1].piece && !curboard->squares [p->square + 2].piece &&
		    curboard->cast [side] [1] && !board_squareattacked (p->square) && !board_squareattacked (p->square + 1) &&
			!board_squareattacked (p->square + 2))
		{
			it->next = move_newnode (piece, 32, sqnum + 2, sqnum);
			it = it->next;
			it->m.special = ms_kcast;
		}

		// queen side
		if (!curboard->squares [p->square - 1].piece && !curboard->squares [p->square - 2].piece &&
		    !curboard->squares [p->square - 3].piece && curboard->cast [side] [0] && !board_squareattacked (p->square) &&
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

movelist *move_newnode (uint8 piece, uint8 taken, uint8 square, uint8 from)
{
	movelist *ret = nodes;
	nodes = nodes->next;

	ret->m.piece = piece;
	ret->m.taken = taken;
	ret->m.square = square;
	ret->m.from = from;
	ret->m.special = 0;
	ret->m.score = curboard->attack [piece];

	ret->next = NULL;

	numnodes ++;

	return ret;
}

// recursively clean a list of moves
void move_clearnodes (movelist *m)
{
	movelist *next;

	if (!m)
		return;

	// iterative version seems to be slower on my lichess-bot machine, but saving it here just in case
/*
	while (m)
	{
		next = m->next;

		m->next = nodes;
		nodes = m;
		m = next;

		numnodes --;
	}
*/
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
	#if NOSORT
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

/*
	// score moves
	it = ret;
	while (it)
	{
		it->m.score = curboard->victim [it->m.taken] + curboard->attack [it->m.piece];
		it = it->next;
	}
*/
	return ret;
}

static char promo (move *m)
{
	switch (m->special)
	{
		case ms_qpromo: return 'q';
		case ms_rpromo: return 'r';
		case ms_bpromo: return 'b';
		case ms_npromo: return 'n';
		default: return '\0';
	}
}

void move_print (move *m, char *c)
{
	snprintf (c, 6, "%c%i%c%i%c", 'a' + board_getfile (m->from), 1 + board_getrank (m->from),
	          'a' + board_getfile (m->square), 1 + board_getrank (m->square), promo (m));
}

// decode algebraic notation into a move struct
void move_decode (const char *c, move *m)
{
	// a1 is 21, each rank adds 10, and each file adds 1
	m->from = 21 + (c [0] - 'a') + (c [1] - '1') * 10;
	m->square = 21 + (c [2] - 'a') + (c [3] - '1') * 10;

	m->piece = curboard->squares [m->from].piece - curboard->pieces;
	if (curboard->squares [m->square].piece)
		m->taken = curboard->squares [m->square].piece - curboard->pieces;
	else
		m->taken = 32;

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

// check for threefold repetition in the history
uint8 move_repcheck (void)
{
	int reps = 0;

	// start from the current position and go backwards
	// searching every other ply saves time since side to move factors into the key, we can discard all moves from the opposing side
	for (int i = htop - 1; i >= 0; i -= 2)
	{
		if (histkeys [i] == poskey)
			reps ++;

		// pawn moves and captures make further repetitions impossible, so we can safely abort
		if (curboard->pieces [history [i].piece].type == pt_pawn || history [i].taken != 32)
			return 0;

		if (reps >= 3)
			return 1;
	}

	return 0;
}
