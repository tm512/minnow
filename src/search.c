#include <stdio.h>

#include "int.h"
#include "board.h"
#include "move.h"
#include "eval.h"

int16 search (movelist *node, uint8 depth, movelist **best)
{
	movelist *it;
	int16 max = -20000, score;

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
		score = -search (it, depth - 1, NULL);
		move_undo (it->m);
//		puts ("undo:");
//		board_print ();
//		usleep (800000);

		if (score > max)
		{
			max = score;
			if (best)
				*best = it;
		}

		it = it->next;
	}

//	if (depth == 4)
//	printf ("best is %i\n", max);
	return max;
}
