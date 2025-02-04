/* Copyright (c) 2014-2022 Kyle Davis, All Rights Reserved.

   Permission is hereby granted, free of charge, to any person
   obtaining a copy of this software and associated documentation
   files (the "Software"), to deal in the Software without
   restriction, including without limitation the rights to use,
   copy, modify, merge, publish, distribute, sublicense, and/or sell
   copies of the Software, and to permit persons to whom the
   Software is furnished to do so, subject to the following
   conditions:

   The above copyright notice and this permission notice shall be
   included in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
   OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
   NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
   HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
   OTHER DEALINGS IN THE SOFTWARE. */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include "ccident.h"
#include "int.h"
#include "uci.h"
#include "board.h"
#include "move.h"
#include "search.h"
#include "timer.h"
#include "eval.h"
#include "hash.h"

const char *startpos = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
char line [8192]; // I think this will be enough to buffer a line

int main (void)
{
	printf ("minnow " GIT_VERSION "\n");
	printf (BUILDTYPE " build, built with " CCIDENT "\n");
	printf ("[c] 2014-2024 Kyle Davis (tm512)\n\n");

	hash_init (DEFAULTHASH * 1024 * 1024);
	move_initnodes ();
	board_initialize (startpos);

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
			htop = 0;
			if (!strncmp (line + 4, "start", 5))
				board_initialize (startpos);
			else
				board_initialize (line + 4);

			board_print ();
		}

		if (!strncmp (line, "perft", 5))
		{
			uint64 start = time_get ();
			uint64 count = perft (atoi (&line [6]), atoi (&line [6]));
			printf ("perft: %llu (took %f seconds)\n", count, time_since_sec (start));
		}

		if (!strncmp (line, "search", 6))
		{
			uint64 start = time_get ();
			int16 score = search (atoi (&line [7]), 0, NULL, 0);
			printf ("score: %i (search took %f seconds)\n", score, time_since_sec (start));
		}

		if (!strncmp (line, "speedtest", 9))
		{
			// run 10 loops, print average speed
			uint64 start = time_get ();
			for (int i = 0; i < 10; i++)
				search (atoi (&line [10]), 0, NULL, 1);
			printf ("avg %f seconds per search\n", time_since_sec (start) / 10.0);
		}

		if (!strncmp (line, "playout", 7))
		{
			uint64 start = time_get ();
			int16 score, nmoves = 0;
			int16 scores[200] = { 0 };
			uint64 hashes[200] = { 0 };
			move best;
			char c[6], moves[200][6] = { 0 };

			contempt = 13333;

			do {
			//	printf ("position %016llX\n", hash_poskey ());
				board_print ();
				hashes[nmoves] = hash_poskey ();

				score = search (atoi (&line [8]), 0, &best, 0);
				scores[nmoves] = score;

				move_print (&best, c);
				move_print (&best, moves[nmoves]);
				printf ("%s as %s (score = %i)\n", c, curboard->side ? "black" : "white", score);
				fflush (stdout);

				move_apply (&best);
				nmoves ++;
			} while (abs (score) != 15000 && abs (score) != contempt && nmoves < 200);

			for (int i = 0; i < nmoves; i++)
			{
				printf ("%03i: hash = %016llX, move = %s, score = %i\n", i, hashes[i], moves[i], scores[i]);
				fflush (stdout);
			}

			printf ("%i-move playout took %f seconds\n", nmoves, time_since_sec (start));
			printf ("final %i\n", score);
			fflush (stdout);
		}

		if (!strncmp (line, "replay", 6))
		{
			int nmoves = 0;
			int side = line [7] == 'w' ? 0 : 1;
			char *args;
			int depth = strtol(&line [9], &args, 10);
			int skip = strtol(args + 1, &args, 10);

			printf ("playing as %s to depth %i, skipping %i:%s\n", side == 0 ? "white" : "black", depth, skip, args);

			while ((args = strstr (args, " ")))
			{
				char *movetype = "";
				char cm[6], cbest[6];
				move m = { 0 }, best = { 0 };

				// check whether it's our color's turn
				if (nmoves % 2 == side && skip == 0)
					search (depth, 0, &best, 0);

				args ++;
				move_decode (args, &m);
				move_print (&m, cm);
				move_print (&best, cbest);

				if (skip > 0)
					movetype = " (skipping)";
				else if (nmoves % 2 != side)
					movetype = " (opponent)";
				else if (strcmp (cm, cbest) != 0)
					movetype = " (mismatch, search != move list)";
				else
					movetype = "";

				printf ("move %i: %s%s\n", nmoves, cm, movetype);
				move_apply (&m);

				if (skip > 0)
					skip --;

				nmoves ++;
			}
		}

		if (!strncmp (line, "history", 7))
			move_printhist ();

		if (!strncmp (line, "hashinfo", 8))
			hash_info ();
		else if (!strncmp (line, "hash", 4))
			hash_init (strtoll (&line [5], NULL, 10) * 1024 * 1024);

		if (!strncmp (line, "disp", 4))
			board_print ();

		line [0] = 0;
	}

	return 0;
}
