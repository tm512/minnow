#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "int.h"
#include "board.h"
#include "move.h"

board *curboard = NULL;

// return the offset into the squares array of a certain coord
inline uint8 board_getsquare (int8 file, int8 rank)
{
	return 21 + rank * 10 + file;
}

inline uint8 board_getfile (uint8 square)
{
	return (square % 10) - 1;
}

inline uint8 board_getrank (uint8 square)
{
	return square / 10 - 2;
}

inline uint8 board_piecesquare (uint8 index)
{
	return 0;
}

void board_initialize (void)
{
	int i, side, pawnrank = 1, backrank = 0;
	// initialize a new game
	// later we'll dispose of the old game here, but we don't care right now

	if (!curboard)
		curboard = malloc (sizeof (board));

	curboard->side = bs_white;

	// set up squares
	for (i = 0; i < 120; i++)
	{
		// padding squares, we use these in move generation
		if (i < 21 || i > 98 || i % 10 == 0 || (i + 1) % 10 == 0)
			curboard->squares [i].padding = 1;
		else
			curboard->squares [i].padding = 0;

		curboard->squares [i].piece = NULL;
	}

	// set up pieces for both sides
	for (side = 0; side < 32; side += 16)
	{
		// set sides, flags
		for (i = 0; i < 16; i++)
		{
			curboard->pieces [side + i].side = !!side;
			curboard->pieces [side + i].flags = 0;
		}

		for (i = 0; i < 8; i++) // pawns, one per file
		{
			curboard->pieces [side + i].type = pt_pawn;
			curboard->pieces [side + i].square = board_getsquare (i, pawnrank);
			curboard->squares [board_getsquare (i, pawnrank)].piece = &curboard->pieces [side + i];
		}

		// set up back rank pieces
		curboard->pieces [side + 8].type = curboard->pieces [side + 9].type = pt_knight;
		curboard->pieces [side + 8].square = board_getsquare (file_b, backrank);
		curboard->squares [board_getsquare (file_b, backrank)].piece = &curboard->pieces [side + 8]; 
		curboard->pieces [side + 9].square = board_getsquare (file_g, backrank);
		curboard->squares [board_getsquare (file_g, backrank)].piece = &curboard->pieces [side + 9]; 

		curboard->pieces [side + 10].type = curboard->pieces [side + 11].type = pt_bishop;
		curboard->pieces [side + 10].square = board_getsquare (file_c, backrank);
		curboard->squares [board_getsquare (file_c, backrank)].piece = &curboard->pieces [side + 10]; 
		curboard->pieces [side + 11].square = board_getsquare (file_f, backrank);
		curboard->squares [board_getsquare (file_f, backrank)].piece = &curboard->pieces [side + 11]; 

		curboard->pieces [side + 12].type = curboard->pieces [side + 13].type = pt_rook;
		curboard->pieces [side + 12].square = board_getsquare (file_a, backrank);
		curboard->squares [board_getsquare (file_a, backrank)].piece = &curboard->pieces [side + 12]; 
		curboard->pieces [side + 13].square = board_getsquare (file_h, backrank);
		curboard->squares [board_getsquare (file_h, backrank)].piece = &curboard->pieces [side + 13]; 

		curboard->pieces [side + 14].type = pt_queen;
		curboard->pieces [side + 14].square = board_getsquare (file_d, backrank);
		curboard->squares [board_getsquare (file_d, backrank)].piece = &curboard->pieces [side + 14]; 

		curboard->pieces [side + 15].type = pt_king;
		curboard->pieces [side + 15].square = board_getsquare (file_e, backrank);
		curboard->squares [board_getsquare (file_e, backrank)].piece = &curboard->pieces [side + 15]; 

		pawnrank += 5;
		backrank += 7;
	}
}

uint8 board_squareattacked (uint8 sq)
{
	movelist *m = move_newnode (NULL), *it;
	move_genlist (m);
	it = m->child;

	while (it)
	{
		if (it->m->square == sq)
		{
			move_clearnodes (m);
			return 1;
		}

		it = it->next;
	}

	move_clearnodes (m);
	return 0;
}

void board_print (void)
{
	int i;
	uint8 rank = 0;
	square *sq;
	printf ("\n  |a|b|c|d|e|f|g|h|");
	for (i = 20; i < 100; i++)
	{
		sq = &curboard->squares [i];
		if (i % 10 == 0)
			printf ("\n|%i|", ++rank);

		if (sq->padding)
			continue;
		else if (sq->piece && (sq->piece->flags & pf_taken) == 0)
		{
			if (!sq->piece->side)
				printf ("\033[1m");

			switch (sq->piece->type)
			{
				case pt_pawn:
					putchar ('p');
				break;
				case pt_knight:
					putchar ('n');
				break;
				case pt_bishop:
					putchar ('b');
				break;
				case pt_rook:
					putchar ('r');
				break;
				case pt_queen:
					putchar ('q');
				break;
				case pt_king:
					putchar ('k');
				break;
			}

			if (!sq->piece->side)
				printf ("\033[0m");
		}
		else
			putchar (' ');

		if ((i + 1) % 10)
			putchar ('|');
	}

	putchar ('\n');
}

int16 search (movelist *node, uint8 depth, movelist **best, int16 alpha, int16 beta);
extern uint64 numnodes;
int main (void)
{
	int i = 0;
	movelist *best;
	board_initialize ();
	board_print ();
	moveroot = move_newnode (NULL);

	while ((curboard->pieces [15].flags & pf_taken) == 0 && (curboard->pieces [31].flags & pf_taken) == 0)
	{
		search (moveroot, 5, &best, -30000, 30000);

		move_make (best);
		board_print ();
		printf ("%u nodes stored (%fMiB)\n", numnodes,
		        (float)(numnodes * (sizeof (movelist) + sizeof (move))) / 1048576.0f);
//		getchar ();
	}

	return 0;
}
