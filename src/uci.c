/* Copyright (c) 2014-2015 Kyle Davis, All Rights Reserved.

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

#define _POSIX_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef _WIN32
	#include <sys/select.h>
#else
	#include <windows.h>
#endif

#include "int.h"
#include "board.h"
#include "move.h"
#include "search.h"
#include "timer.h"
#include "hash.h"
#include "uci.h"

extern const char *startpos;
extern char line [8192];

int uci_main (void)
{
	setbuf (stdin, NULL);
	setbuf (stdout, NULL);

	printf ("\nid name minnow " GIT_VERSION "\n");
	printf ("id author tm512\n");
	printf ("uciok\n");
	fflush (stdout);

	while (uci_parse (0));

	return 0;
}

uint8 uci_parse (uint8 searching)
{
	fgets (line, 8192, stdin);

	if (!strncmp (line, "isready", 7))
		printf ("readyok\n");

	if (!strncmp (line, "ucinewgame", 10))
		hbot = htop = 0;

	if (!strncmp (line, "position", 8))
	{
		char *posline = line + 9;

		if (!strncmp (posline, "startpos", 8))
		{
			// assume new game
			hbot = htop = 0;
			board_initialize (startpos);
		}
		else if (!strncmp (posline, "fen", 3))
		{
			// assume we're making a move here
			hbot ++;
			htop = 0;
			board_initialize (posline + 4);
		}

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

		if (searching)
			return 0;
	}

	if (!strncmp (line, "go", 2))
	{
		move best;
		char c [6];
		uint64 depth = 0, wtime = 0, btime = 0, maxtime = 0;
		char *cdepth, *cwtime, *cbtime, *cperft;

		// determine some search options
		cdepth = strstr (line, "depth");
		cwtime = strstr (line, "wtime");
		cbtime = strstr (line, "btime");
		cperft = strstr (line, "perft");

		if (cperft)
		{
			uint64 start = time_get ();
			uint64 count;

			depth = atoi (cperft + 6);
			count = perft (depth, depth);
			printf ("perft: %llu (took %f seconds)\n", count, time_since_sec (start));
			return 1;
		}

		if (cdepth)
			depth = atoi (cdepth + 6);

		if (cwtime)
			wtime = atoi (cwtime + 6);

		if (cbtime)
			btime = atoi (cbtime + 6);

		if (depth == 1)
			depth = 2; // never search less than 2 deep

		if (cwtime && cbtime)
		{
			if (curboard->side)
				maxtime = time_alloc (btime, wtime);
			else
				maxtime = time_alloc (wtime, btime);
		}

		search (depth, maxtime, &best, 0);
		move_print (&best, c);
		printf ("bestmove %s\n", c);
	}

	if (!strncmp (line, "disp", 4))
	{
		board_print ();
		printf ("key: %16llX\n", hash_poskey ());
	}

	if (!strncmp (line, "quit", 4))
		exit (0);

	if (!strncmp (line, "stop", 4) && searching)
		return 0;

	fflush (stdout);

	return 1;
}

uint8 uci_peek (void)
{
	#ifndef _WIN32
	fd_set rfs;
	struct timeval t = { 0 };

	FD_ZERO (&rfs);
	FD_SET (fileno (stdin), &rfs);
	select (16, &rfs, 0, 0, &t);

	return FD_ISSET (fileno (stdin), &rfs);
	#else
	HANDLE stdin_h = GetStdHandle (STD_INPUT_HANDLE);
	if (GetConsoleMode(stdin_h, NULL))
		return 0; // meh

	DWORD avail;
	PeekNamedPipe (stdin_h, NULL, 0, 0, &avail, 0);
	return avail > 0;
	#endif
}
