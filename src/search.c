#include <stdio.h>
#include <string.h>

#include "int.h"
#include "board.h"
#include "move.h"
#include "eval.h"
#include "timer.h"
#include "search.h"

uint64 leafnodes = 0;
uint64 endtime = 0;
uint64 iterations = 0;
int16 absearch (uint8 depth, uint8 start, pvlist *pv, pvlist *oldpv, int16 alpha, int16 beta)
{
	movelist *m, *it, pvm;
	pvlist stackpv = { 0 };
	int16 score;

	// check if we're out of time every so often
	if (!(++iterations & 8191) && time_get () >= endtime)
	{
		pv->nodes = 0;
		return 31000;
	}

	if (depth == 0)
	{
		leafnodes ++;
		pv->nodes = 0;
		return evaluate ();
	}

	m = move_genlist ();

	if (oldpv && oldpv->nodes > 0 && depth > 1)
	{
		pvm.m = oldpv->moves [start - depth];
		pvm.next = m;
		m = &pvm;
	}

	it = m;

	while (it)
	{
		if (it->m.taken == curboard->kings [!curboard->side] - curboard->pieces)
		{
			if (m == &pvm)
				m = m->next;

			move_clearnodes (m);

			return 15000 + depth;
		}

		move_apply (&it->m);

		if (it == &pvm)
			score = -absearch (depth - 1, start, &stackpv, oldpv, -beta, -alpha);
		else
			score = -absearch (depth - 1, start, &stackpv, NULL, -beta, -alpha);

		if (score == -31000)
		{
			if (m == &pvm)
				m = m->next;

			move_clearnodes (m);

			pv->nodes = 0;
			return 31000;
		}

		#if 0
		if (depth == start)
		{
			char notation [6];
			move_print (&it->m, notation);
			printf ("%s: %i\n", notation, score);
		}
		#endif

		#if 1
		if (depth != start && score >= beta)
		{
			move_undo (&it->m);

			if (m == &pvm)
				m = m->next;

			move_clearnodes (m);

			return beta;
		}
		#endif

		// if we're considering this move, make sure it is legal
		if (score > alpha)
		{
			alpha = score;

			pv->moves [0] = it->m;
			memcpy (pv->moves + 1, stackpv.moves, stackpv.nodes * sizeof (move));
			pv->nodes = stackpv.nodes + 1;
		}

		move_undo (&it->m);
		it = it->next;
	}

	if (m == &pvm)
		m = m->next;

	move_clearnodes (m);

	return alpha;
}

// iterative deepening
int16 search (uint8 depth, uint64 wtime, uint64 btime, move *best)
{
	int i, j;
	int16 ret;
	uint64 start, maxtime, ittime;

	pvlist oldpv = { 0 };

	// Search "indefinitely"
	if (depth == 0)
		depth = 255;

	if (wtime > 0) // determine how much time to spend on this move
	{
		maxtime = wtime / 10; // start with 1/10th of the time we have left

		if (wtime - maxtime > btime) // if we'd still have more time than our opponent after this move, give some extra
			maxtime += (wtime - btime - maxtime) / 2;

		endtime = time_get () + maxtime;
		printf ("using %u\n", maxtime);
	}
	else
		endtime = ~0; // never end

	for (i = 1; i <= depth; i++)
	{
		start = time_get ();
		pvlist pv = { 0 };
		ret = absearch (i, i, &pv, &oldpv, -30000, 30000);

		ittime = time_since (start);

		if (pv.nodes > 0)
		{
			oldpv = pv;

			if (ittime == 0)
				ittime = 1;

			// print UCI info
			printf ("info depth %u score cp %i time %u nodes %u nps %u pv ", i, ret, ittime, leafnodes, (leafnodes * 1000) / ittime);

			for (j = 0; j < oldpv.nodes; j++)
			{
				char notation [6];
				move_print (&oldpv.moves [j], notation);
				printf ("%s ", notation);
			}

			printf ("\n");
		}

		leafnodes = 0;

		if (time_get () >= endtime)
			break;
	}

	if (best)
		*best = oldpv.moves [0];

	return ret;
}

uint64 castles = 0, promos = 0;
uint64 perft (uint8 depth, uint8 start)
{
	uint64 count = 0;
	movelist *it, *m;

	if (depth == 0)
		return 1;

	m = move_genlist ();
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
				//printf ("%s: %u nodes\n", notation, add);
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
		printf ("%u castles, %u promotions\n", castles, promos);

	return count;
}
