#include <stdio.h>

#include "int.h"
#include "board.h"
#include "move.h"
#include "eval.h"

int illegal = 0;
int16 search (movelist *node, uint8 depth, movelist **best, int16 alpha, int16 beta)
{
	movelist *it, *prev = NULL;
	int16 score;

	if (depth == 0)
		return evaluate ();

	if (!node->child)
		move_genlist (node);

	it = node->child;

	while (it)
	{
//		printf ("%x\n", it->m);
		move_apply (&it->m);
//		if (board_squareattacked (curboard->pieces [!curboard->side * 16 + 15].square))
//			illegal ++;
//		puts ("apply:");
//		board_print ();
//		usleep (800000);
		score = -search (it, depth - 1, NULL, -beta, -alpha);
//		puts ("undo:");
//		board_print ();
//		usleep (800000);

		#if 1
		if (score >= beta)
		{
			move_undo (&it->m);
			return beta;
		}
		#endif

		// if we're considering this move, make sure it is legal
		if (score > alpha && !board_squareattacked (curboard->pieces [!curboard->side * 16 + 15].square))
		{
			alpha = score;
			if (best)
				*best = it;

			// put this node at the beginning of the list, this sorts on the fly
			if (it != node->child)
			{
				prev->next = it->next;
				it->next = node->child;
				node->child = it;

				move_undo (&it->m);
				it = prev->next;
				continue;
			}
		}

		move_undo (&it->m);
		prev = it;
		it = it->next;
	}

//	if (depth == 4)
//	printf ("%i illegal moves\n", illegal);
	return alpha;
}
