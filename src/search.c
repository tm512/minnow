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
#include "eval.h"
#include "timer.h"
#include "uci.h"
#include "hash.h"
#include "values.h"
#include "search.h"

// toggle null move heuristic
// thought to be the source of illegal moves, it seems like those were actually caused by the search using hash moves
// still, if search is having issues, try disabling this
#define NULLMOVE 1

uint64 leafnodes = 0;
uint64 endtime = 0;
uint64 iterations = 0;

static int sanitycheck (move *m, piece **mover, piece **target)
{
	*mover = curboard->squares[m->from].piece;
	if (!(*mover) || curboard->pieces [m->piece].type != (*mover)->type)
	{
		printf ("sanitycheck: failed mover check (square: %i, piece list: %i)\n",
		        *mover ? (*mover)->type : -1, curboard->pieces [m->piece].type);
		return 0;
	}

	if (m->special == ms_enpascap && curboard->enpas)
		*target = curboard->enpas;
	else
		*target = curboard->squares [m->square].piece;

	if (m->taken < 32 && (!(*target) || curboard->pieces [m->taken].type != (*target)->type))
	{
		printf ("sanitycheck: failed target check (square: %i, piece list: %i)\n",
		        *target ? (*target)->type : -1, curboard->pieces [m->taken].type);
		return 0;
	}

	return 1;
}

int16 absearch (uint8 depth, uint8 start, pvlist *pv, int16 alpha, int16 beta, uint8 donull)
{
	movelist *m = NULL, *it = NULL, hbm;
	pvlist stackpv = { 0 };
	int16 score, bestscore = -30000;
	move *hashbest = NULL, *bestmove = NULL;
	uint8 storetype = et_alpha, probetype = et_null;
	uint8 minors, majors;
	uint16 idx, legalmoves = 0;

	// occasionally check if we need to abort
	if (!(++iterations & 8191))
	{
		uint8 ret = 1;

		// see if we have input waiting on stdin
		while (uci_peek () && ret)
			ret = uci_parse (1);

		if (time_get () >= endtime || !ret)
		{
			pv->nodes = 0;
			return 31000;
		}
	}

	if (depth == 0)
	{
		leafnodes ++;
		pv->nodes = 0;
		score = quies (alpha, beta);
		hash_store (depth, score, et_exact, NULL);
		return score;
	}

	// try null move
	#if NULLMOVE
	minors = curboard->piececount [!curboard->side] [pt_knight] + curboard->piececount [!curboard->side] [pt_bishop];
	majors = curboard->piececount [!curboard->side] [pt_rook] + curboard->piececount [!curboard->side] [pt_queen];
	curboard->side = !curboard->side;
	if (donull && depth >= 3 && curboard->piececount [!curboard->side] [pt_pawn] > 1 && (minors > 1 || majors > 0) &&
	    !board_squareattacked (curboard->kings [!curboard->side]->square))
	{
		pvlist nullpv = { 0 };
		move_applynull ();
		score = -absearch (depth - 1 - 2, depth - 1 - 2, &nullpv, -beta, -beta + 1, 0);
		move_undonull ();

		if (score >= beta && abs (score) != 20000)
		{
			curboard->side = !curboard->side;
			return beta;
		}
	}
	curboard->side = !curboard->side;
	#endif

	score = hash_probe (depth, alpha, beta, &probetype, &hashbest);

	if (probetype != et_exact)
	{
		// see if probe returned upper/lower bound
		if (score != -32000)
			return score;

		// otherwise, we had a TT miss or the depth wasn't high enough, and we need to generate moves
		it = m = move_order (move_genlist ());
	}

	if (hashbest)
	{
		piece *mover, *target;

		if (sanitycheck (hashbest, &mover, &target))
		{
			hbm.m = *hashbest;

			// update mover and target (where applicable)
			hbm.m.piece = mover - curboard->pieces;
			hbm.m.taken = target ? target - curboard->pieces : 32;

			hbm.next = it;
			it = &hbm;
		}
		else
		{
			char c [6];
			move_print (hashbest, c);
			printf ("Illegal hash move: %s, (pseudo-)legal movelist: ", c, hash_poskey ());
			while (it)
			{
				move_print (&it->m, c);
				printf ("%s ", c);
				it = it->next;
			}

			it = m;
			printf ("\nhistory:\n");
			move_printhist ();
		//	board_print ();
		}
	}

	while (it)
	{
		if (it->m.taken == curboard->kings [!curboard->side] - curboard->pieces)
		{
			move_clearnodes (m);
			return 20000;
		}

		move_apply (&it->m);

		if (legalmoves == 0 && board_squareattacked (curboard->kings [!curboard->side]->square))
		{
			move_undo (&it->m);
			it = it->next;
			continue;
		}

		legalmoves ++;

		if (move_repcheck ())
			score = contempt;
		else
			score = -absearch (depth - 1, start, &stackpv, -beta, -alpha, 1);

		if (score == -31000 || (!donull && score == -20000))
		{
			move_undo (&it->m);
			move_clearnodes (m);

			pv->nodes = 0;
			return -score;
		}

		// seperate from alpha, keep track of the best move from this position, for the hash table
		if (score > bestscore)
		{
			bestscore = score;
			bestmove = &it->m;
		}

		if (score >= beta && depth != start)
		{
			move_undo (&it->m);

			hash_store (depth, beta, et_beta, bestmove);
			move_clearnodes (m);

			return beta;
		}

		if (score > alpha)
		{
			alpha = score;
			storetype = et_exact;

			pv->moves [0] = it->m;
			memcpy (pv->moves + 1, stackpv.moves, stackpv.nodes * sizeof (move));
			pv->nodes = stackpv.nodes + 1;
		}

		move_undo (&it->m);
		it = it->next;
	}

	// no legal moves available. check for mate
	if (legalmoves == 0)
	{
		curboard->side = !curboard->side;

		move_clearnodes (m);
		if (board_squareattacked (curboard->kings [!curboard->side]->square))
		{
			curboard->side = !curboard->side;
			return -15000;
		}
		else
		{
			curboard->side = !curboard->side;
			return contempt;
		}
	}

	hash_store (depth, alpha, storetype, bestmove);
	move_clearnodes (m);

	return alpha;
}

int16 quies (int16 alpha, int16 beta)
{
	movelist *m, *it;
	int16 score = evaluate (alpha, beta);

	if (score >= beta)
		return beta;

	if (score > alpha)
		alpha = score;

	m = it = move_order (move_genlist ());

	while (it && it->m.taken != 32)
	{
		if (it->m.taken == curboard->kings [!curboard->side] - curboard->pieces)
		{
			move_clearnodes (m);
			return 20000;
		}

		move_apply (&it->m);

		score = -quies (-beta, -alpha);

		if (score >= beta)
		{
			move_undo (&it->m);
			move_clearnodes (m);

			return beta;
		}

		if (score > alpha)
			alpha = score;

		move_undo (&it->m);
		it = it->next;
	}

	move_clearnodes (m);

	return alpha;
}

// iterative deepening
int16 search (uint8 depth, uint64 maxtime, move *best, int hashclear)
{
	int16 ret = 0, oldret;
	uint64 start, ittime;

	// Search "indefinitely"
	// TODO: stack overflows occur around ply 65 with default Linux stack size, so cap this to a (hopefully) safe maximum
	if (depth == 0 || depth > 60)
		depth = 60;

	if (maxtime > 0) // determine how much time to spend on this move
		endtime = time_get () + maxtime;
	else
		endtime = ~0; // never end

	for (int i = 1; i <= depth && abs (ret) < 15000; i++)
	{
		start = time_get ();
		pvlist pv = { 0 };
		ret = absearch (i, i, &pv, -30000, 30000, 1);

		// absearch's result is relative to the current side, whereas most UCI stuff expects white-relative scores
		ret = curboard->side == bs_white ? ret : -ret;

		ittime = time_since (start);

		if (pv.nodes > 0)
		{
			int infotime = ittime > 0 ? ittime : 1;

			// print UCI info
			printf ("info depth %u score cp %i time %u nodes %u nps %u pv ", i, ret, infotime, leafnodes, (leafnodes * 1000) / infotime);

			for (int j = 0; j < pv.nodes; j++)
			{
				char notation [6];
				move_print (&pv.moves [j], notation);
				printf ("%s ", notation);
			}

			printf ("\n");
			fflush (stdout);

			if (best)
				*best = pv.moves [0];
		}

		leafnodes = 0;

		// if the last search returned +/- 31000, it was aborted
		if (ret == 31000 || ret == -31000)
			break;

		oldret = ret;

		// since each iteration takes longer than the last, don't search deeper if we couldn't repeat this last search twice
		if (time_get () >= endtime - ittime * 2)
			break;
	}

	if (hashclear)
		hash_clear ();

	return oldret;
}

uint64 castles = 0, promos = 0;
uint64 perft (uint8 depth, uint8 start)
{
	uint64 count = 0;
	movelist *it, *m;

	if (depth == 0)
		return 1;

	m = move_order (move_genlist ());
	it = m;

	while (it)
	{
		move_apply (&it->m);

		if (!board_squareattacked (curboard->kings [!curboard->side]->square))
		{
			uint64 add = perft (depth - 1, start);
			count += add;

			if (depth == start)
			{
				char notation [6];
				move_print (&it->m, notation);
				printf ("%s %u\n", notation, add);
			}

			if (depth == 1 && (it->m.special == ms_kcast || it->m.special == ms_qcast))
				castles ++;
			if (depth == 1 && it->m.special >= ms_qpromo && it->m.special <= ms_npromo)
				promos ++;
		}

		move_undo (&it->m);
		it = it->next;
	}

	move_clearnodes (m);

	if (depth == start)
	{
		printf ("%llu castles, %llu promotions\n", castles, promos);
		castles = promos = 0;
	}

	return count;
}
