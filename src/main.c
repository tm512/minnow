#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include "int.h"
#include "uci.h"
#include "board.h"
#include "move.h"
#include "search.h"
#include "timer.h"
#include "eval.h"

const char *startpos = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
extern uint64 leafnodes;

int main (void)
{
	char line [8192]; // I think this will be enough to buffer a line

	printf ("minnow " GIT_VERSION "\n");
	printf ("[c] 2014 Kyle Davis (tm512)\n\n");

	move_initnodes ();

	while (1)
	{
		printf ("> ");
		fgets (line, 8192, stdin);

		if (!strncmp (line, "quit", 4))
			break;

		if (!strncmp (line, "uci", 3))
			return uci_main ();

		if (!strncmp (line, "pos", 3))
		{
			if (!strncmp (line + 4, "startpos", 8))
				board_initialize (startpos);
			else
				board_initialize (line + 4);

			board_print ();
		}

		if (!strncmp (line, "perft", 5))
		{
			clock_t start = clock ();
			uint64 count = perft (atoi (&line [6]), atoi (&line [6]));
			printf ("perft: %i (took %f seconds)\n", count, (float)(clock () - start) / CLOCKS_PER_SEC);
		}

		if (!strncmp (line, "search", 6))
		{
			uint64 start = time_get ();
			int16 score = search (atoi (&line [7]), 0, NULL);
			printf ("score: %i (search took %f seconds, %u leaf nodes)\n", score, time_since_sec (start), leafnodes);
			leafnodes = 0;
		}

		line [0] = 0;
	}

	return 0;
}
