#include <stdio.h>

#include "int.h"
#include "board.h"
#include "move.h"
#include "eval.h"

uint64 leafnodes = 0;
int16 absearch (uint8 depth, move *best, int16 alpha, int16 beta)
{
	movelist *m, *it, *prev = NULL;
	move newbest;
	int16 score;

	if (depth == 0)
	{
		leafnodes ++;
		return evaluate ();
	}

	m = move_genlist ();
	it = m;

	while (it)
	{
		move_apply (&it->m);

		score = -absearch (depth - 1, NULL, -beta, -alpha);

		if (depth == 0)
		{
			char notation [6];
			move_print (&it->m, notation);
			printf ("%s: %i (a = %i, b = %i)\n", notation, score, alpha, beta);
		}

		#if 1
		if (score >= beta)
		{
			if (best)
				*best = it->m;

			move_undo (&it->m);
			move_clearnodes (m);

			return beta;
		}
		#endif

		// if we're considering this move, make sure it is legal
		if (score > alpha && !board_squareattacked (curboard->kings [!curboard->side]->square))
		{
			alpha = score;
			if (best)
				newbest = it->m;
		}

		move_undo (&it->m);
		prev = it;
		it = it->next;
	}

	move_clearnodes (m);

	if (best)
		*best = newbest;

	return alpha;
}

// iterative deepening
void search (uint8 depth, move *best)
{
	int i;
	move newbest;
	for (i = 1; i <= depth; i++)
	{
		newbest.piece = 33;
		absearch (i, &newbest, -30000, 30000);
		if (best && newbest.piece != 33)
			*best = newbest;
	}
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
