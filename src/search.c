#include <stdio.h>

#include "int.h"
#include "board.h"
#include "move.h"
#include "eval.h"

#define USEBETA

int16 search (movelist *node, uint8 depth, movelist **best, int16 alpha, int16 beta)
{
	movelist *it;
	int16 score;

	if (depth == 0)
		return evaluate ();

	if (!node->child)
		move_genlist (node);

	it = node->child;

	while (it)
	{
//		printf ("%x\n", it->m);
		move_apply (it->m);
//		puts ("apply:");
//		board_print ();
//		usleep (800000);
		score = -search (it, depth - 1, NULL, -beta, -alpha);
//		puts ("undo:");
//		board_print ();
//		usleep (800000);

		#ifdef USEBETA
		if (score >= beta)
		{
			move_undo (it->m);
			return beta;
		}
		#endif

		if (score > alpha)
		{
			alpha = score;
			if (best)
				*best = it;
		}

		move_undo (it->m);
		it = it->next;
	}

	return alpha;
}
