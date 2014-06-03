#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "int.h"
#include "board.h"
#include "move.h"
#include "search.h"

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

piece dummy = { .flags = pf_moved };

void board_initialize (const char *fen)
{
	int i;
	uint8 rank = 91, file = 0;
	const char *it = fen;
	// track indexes into the piece array
	uint8 idx [2] = { 0, 16 };

	if (curboard)
		free (curboard);

	curboard = malloc (sizeof (board));

	// initialize castling stuff
	curboard->cast [0] [0] = curboard->cast [0] [1] = 0;
	curboard->cast [1] [0] = curboard->cast [1] [1] = 0;
	curboard->rooks [0] [0] = curboard->rooks [0] [1] = &dummy;
	curboard->rooks [1] [0] = curboard->rooks [1] [1] = &dummy;

	// initialize squares
	for (i = 0; i < 120; i++)
	{
		// padding squares, we use these in move generation
		if (i < 21 || i > 98 || i % 10 == 0 || (i + 1) % 10 == 0)
			curboard->squares [i].padding = 1;
		else
			curboard->squares [i].padding = 0;

		curboard->squares [i].piece = NULL;
	}

	// initialize pieces
	for (i = 0; i < 32; i++)
	{
		curboard->pieces [i].type = pt_none;
		curboard->pieces [i].side = (i > 15);
		curboard->pieces [i].flags = 0;
		curboard->pieces [i].square = 0;
	}

	// start walking through the string

	// parse position information
	while (*it != ' ')
	{
		if (*it > '0' && *it < '9') // character specifies spaces
		{
			// get uint from ascii
			uint8 spaces = *it - '0';
			file += spaces;
		}
		else if (*it == '/') // move down a rank
		{
			file = 0;
			rank -= 10;
		}
		else // this specifies a piece
		{
			char p = *it;
			uint8 side = (p >= 'a'); // black pieces are lowercase and 'a' > 'A'

			if (side) // set the piece to uppercase if black, this simplifies things
				p -= 32;

			switch (p)
			{
				case 'P':
					curboard->pieces [idx [side]].type = pt_pawn;
				break;
				case 'N':
					curboard->pieces [idx [side]].type = pt_knight;
				break;
				case 'B':
					curboard->pieces [idx [side]].type = pt_bishop;
				break;
				case 'R':
					curboard->pieces [idx [side]].type = pt_rook;
				break;
				case 'Q':
					curboard->pieces [idx [side]].type = pt_queen;
				break;
				case 'K':
					curboard->pieces [idx [side]].type = pt_king;
				break;
			}

			// mark pawns that have left the first rank as moved
			if (p == 'P' && ((!side && rank != 31) || (side && rank != 81)))
					curboard->pieces [idx [side]].flags |= pf_moved;

			// we need to track kings and rooks for castling and mating
			if (p == 'K')
				curboard->kings [side] = &curboard->pieces [idx [side]];

			if (p == 'R')
			{
				if (file == 0)
					curboard->rooks [side] [0] = &curboard->pieces [idx [side]];
				else if (file == 7)
					curboard->rooks [side] [1] = &curboard->pieces [idx [side]];
			}
			

			curboard->pieces [idx [side]].square = rank + file;
			curboard->squares [rank + file].piece = &curboard->pieces [idx [side]];
			idx [side] ++;
			file ++;
		}

		it ++;
	}

	// see whose move it is
	curboard->side = (*(++it) == 'b');
	it += 2;

	// determine castling rights
	while (*it != ' ')
	{
		switch (*it)
		{
			case 'K':
				curboard->cast [0] [1] = 1;
			break;
			case 'Q':
				curboard->cast [0] [0] = 1;
			break;
			case 'k':
				curboard->cast [1] [1] = 1;
			break;
			case 'q':
				curboard->cast [1] [0] = 1;
			break;
			default: break;
		}

		it ++;
	}
}

uint8 attackhack = 0;
uint8 board_squareattacked (uint8 sq)
{
	// this is a hack:
	attackhack = 1;

	movelist *m = move_genlist (), *it;
	it = m;

	while (it)
	{
		if (it->m.square == sq)
		{
			move_clearnodes (m);
			attackhack = 0;
			return 1;
		}

		it = it->next;
	}

	move_clearnodes (m);
	attackhack = 0;
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
