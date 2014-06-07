#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "int.h"
#include "board.h"
#include "move.h"
#include "search.h"

extern const char *startpos;
int uci_main (void)
{
	char line [8192];

	printf ("id name minnow " GIT_VERSION "\n");
	printf ("id author tm512\n");
	printf ("uciok\n");
	fflush (stdout);

	while (1)
	{
		fgets (line, 8192, stdin);

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
			uint64 depth = 0, wtime = 0, btime = 0;
			char *cdepth, *cwtime, *cbtime;

			// determine some search options
			cdepth = strstr (line, "depth");
			cwtime = strstr (line, "wtime");
			cbtime = strstr (line, "btime");

			if (cdepth)
				depth = atoi (cdepth + 6);

			if (cwtime)
				wtime = atoi (cwtime + 6);

			if (cbtime)
				btime = atoi (cbtime + 6);

			if (depth == 2)
				depth = 2; // never search less than 2 deep

			search (depth, wtime, btime, &best);
			move_print (&best, c);
			printf ("bestmove %s\n", c);
		}

		if (!strncmp (line, "quit", 4))
			return 0;

		fflush (stdout);
	}

	return 0;
}
