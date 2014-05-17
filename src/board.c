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
	uint16 piece = curboard->pieces [index];
	return board_getsquare ((piece & 0x38) >> 3, (piece & 0x1c0) >> 6);
}

void board_initialize (void)
{
	int i, side, pawnrank = 1, backrank = 0;
	// initialize a new game
	// later we'll dispose of the old game here, but we don't care right now

	if (!curboard)
		curboard = malloc (sizeof (board));

	curboard->flags = 0;
	memset (curboard->pieces, 0, 64); // clear piece information

	curboard->parent = curboard->children = curboard->next = NULL;

	// set up squares
	for (i = 0; i < 120; i++)
	{
		curboard->squares [i] = 0;
		// padding squares, we use these in move generation
		if (i < 21 || i > 98 || i % 10 == 0 || (i + 1) % 10 == 0)
			curboard->squares [i] |= sf_padding;

		// white's starting position
		if (i >= 21 && i <= 38)
			curboard->squares [i] |= sf_wocc;

		// black's starting position
		if (i >= 81 && i <= 98)
			curboard->squares [i] |= sf_bocc;
	}

	// set up pieces for both sides
	for (side = 0; side < 32; side += 16)
	{
		for (i = 0; i < 8; i++) // pawns, one per file
		{
			curboard->pieces [side + i] = pt_pawn | (i << 3) | (pawnrank << 6);
			curboard->squares [board_getsquare (i, pawnrank)] |= side + i;
		}

		// set up back rank pieces
		curboard->pieces [side + 8] = pt_knight | (file_b << 3) | (backrank << 6);
		curboard->squares [board_getsquare (file_b, backrank)] |= side + 8; 

		curboard->pieces [side + 9] = pt_knight | (file_g << 3) | (backrank << 6);
		curboard->squares [board_getsquare (file_g, backrank)] |= side + 9; 

		curboard->pieces [side + 10] = pt_bishop | (file_c << 3) | (backrank << 6);
		curboard->squares [board_getsquare (file_c, backrank)] |= side + 10; 

		curboard->pieces [side + 11] = pt_bishop | (file_f << 3) | (backrank << 6);
		curboard->squares [board_getsquare (file_f, backrank)] |= side + 11; 

		curboard->pieces [side + 12] = pt_rook | (file_a << 3) | (backrank << 6);
		curboard->squares [board_getsquare (file_a, backrank)] |= side + 12; 

		curboard->pieces [side + 13] = pt_rook | (file_h << 3) | (backrank << 6);
		curboard->squares [board_getsquare (file_h, backrank)] |= side + 13; 

		curboard->pieces [side + 14] = pt_queen | (file_d << 3) | (backrank << 6);
		curboard->squares [board_getsquare (file_d, backrank)] |= side + 14; 

		curboard->pieces [side + 15] = pt_king | (file_e << 3) | (backrank << 6);
		curboard->squares [board_getsquare (file_e, backrank)] |= side + 15; 

		pawnrank += 5;
		backrank += 7;
	}
}

void board_print (void)
{
	int i;
	uint8 sq;
	for (i = 20; i < 100; i++)
	{
		sq = curboard->squares [i];
		if (i % 10 == 0)
			printf ("\n|");

		if (sq & sf_padding)
			continue;
		else if (sq & sf_bocc || sq & sf_wocc)
		{
			if (sq & sf_wocc)
				printf ("\033[1m");

			switch (curboard->pieces [sq & 0x1f] & 0x07)
			{
				case pt_pawn:
					putchar ('p');
				break;
				case pt_knight:
					putchar ('k');
				break;
				case pt_bishop:
					putchar ('b');
				break;
				case pt_rook:
					putchar ('r');
				break;
				case pt_queen:
					putchar ('Q');
				break;
				case pt_king:
					putchar ('K');
				break;
			}

			if (sq & sf_wocc)
				printf ("\033[0m");
		}
		else
			putchar (' ');

		if ((i + 1) % 10)
			putchar ('|');
	}

	putchar ('\n');
}

int16 search (movelist *node, uint8 depth, movelist **best);
int main (void)
{
	int st;
	movelist *best;
	board_initialize ();
	moveroot = move_newnode (NULL);

	while ((curboard->pieces [15] & pf_taken) == 0 && (curboard->pieces [31] & pf_taken) == 0)
	{
		search (moveroot, 4, &best);

		move_make (best);
		board_print ();
	}

	return 0;
}
