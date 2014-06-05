#include <stdio.h>
#include <string.h>

#include "int.h"
#include "board.h"
#include "move.h"
#include "eval.h"
#include "search.h"

uint64 leafnodes = 0;
int16 absearch (uint8 depth, uint8 start, pvlist *pv, pvlist *oldpv, int16 alpha, int16 beta)
{
	movelist *m, *it, pvm;
	pvlist stackpv = { 0 };
	int16 score, oldalpha = alpha;

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
		move_apply (&it->m);

		#if 0
		if (depth == start)
		{
			char notation [6];
			move_print (&it->m, notation);
			printf ("%s\n", notation);
		}
		#endif

		if (it == &pvm)
			score = -absearch (depth - 1, start, &stackpv, oldpv, -beta, -alpha);
		else
			score = -absearch (depth - 1, start, &stackpv, NULL, -beta, -alpha);

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
		if (((alpha == oldalpha && score >= alpha) || score > alpha) && !board_squareattacked (curboard->kings [!curboard->side]->square))
		{
			alpha = score;

//			if (depth == start)
//				printf ("depth %u: alpha raised to %i from %i\n", depth, score, alpha);

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

	if (alpha == oldalpha && board_squareattacked (curboard->kings [curboard->side]->square))
		return -15000 - depth; // subtract the depth so that sooner checkmates score higher

	return alpha;
}

// iterative deepening
int16 search (uint8 depth, move *best)
{
	int i, j;
	int16 ret;
	pvlist oldpv = { 0 };

	leafnodes = 0;

	for (i = 1; i <= depth; i++)
	{
		pvlist pv = { 0 };
		ret = absearch (i, i, &pv, &oldpv, -30000, 30000);
		oldpv = pv;
	}

	for (j = 0; j < oldpv.nodes; j++)
	{
		char notation [6];
		move_print (&oldpv.moves [j], notation);
		printf ("pv (%u): %s\n", j, notation);
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
