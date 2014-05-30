#include <stdio.h>
#include <string.h>

#include "int.h"
#include "board.h"
#include "move.h"
#include "search.h"

extern const char *startpos;
int uci_main (void)
{
	char line [8192];
//	FILE *log = fopen ("/tmp/minnow.log", "w");

	printf ("id name minnow " GIT_VERSION "\n");
	printf ("id author tm512\n");
	printf ("uciok\n");
	fflush (stdout);

	while (1)
	{
		fgets (line, 8192, stdin);
//		fprintf (log, "%s\n", line);

		if (!strncmp (line, "isready", 7))
			printf ("readyok\n");

		if (!strncmp (line, "position", 8))
		{
			char *posline = line + 9;

			if (!strncmp (posline, "startpos", 8))
				board_initialize (startpos);
			else if (!strncmp (posline, "fen", 3))
				board_initialize (posline + 4);

			// need to make some moves on this position as well?
			posline = strstr (line, "moves");

			if (posline)
			{
				posline += 6;

				while (1)
				{
					move m = { 0 };
					move_decode (posline, &m);
					move_make (&m);
					// board_print ();

					posline = strstr (posline, " ");

					if (!posline)
						break;

					posline ++;
				}
			}
		}

		if (!strncmp (line, "go", 2))
		{
			move best;
			char c [6];
			search (6, &best);
			move_print (&best, c);
			printf ("bestmove %s\n", c);
		}

		if (!strncmp (line, "quit", 4))
			return 0;

		fflush (stdout);
	}

	return 0;
}
