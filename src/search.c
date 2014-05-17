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
		score = search (it, depth - 1, NULL);
		move_undo (it->m);

		if (score > max)
		{
			max = score;
			if (best)
				*best = it;
		}

		it = it->next;
	}

	return max;
}
