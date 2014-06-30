/* Copyright (c) 2014 Kyle Davis, All Rights Reserved.

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
#include <stdlib.h>
#include <string.h>

#include "int.h"
#include "board.h"
#include "move.h"
#include "search.h"
#include "values.h"
#include "hash.h"

board *curboard = NULL;
//static uint64 poskey;

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
	uint8 rank = 91, file = 0;
	const char *it = fen;
	// track indexes into the piece array
	uint8 idx [2] = { 0, 16 };

	if (curboard)
		free (curboard);

	curboard = malloc (sizeof (board));
	htop = 0;
	poskey = 0;

	// initialize castling stuff
	curboard->cast [0] [0] = curboard->cast [0] [1] = 0;
	curboard->cast [1] [0] = curboard->cast [1] [1] = 0;
	curboard->rooks [0] [0] = curboard->rooks [0] [1] = &dummy;
	curboard->rooks [1] [0] = curboard->rooks [1] [1] = &dummy;

	curboard->enpas = NULL;

	curboard->score [0] = curboard->score [1] = 0;

	// initialize squares
	for (int i = 0; i < 120; i++)
	{
		// padding squares, we use these in move generation
		if (i < 21 || i > 98 || i % 10 == 0 || (i + 1) % 10 == 0)
			curboard->squares [i].padding = 1;
		else
			curboard->squares [i].padding = 0;

		curboard->squares [i].piece = NULL;
	}

	// initialize pieces and sort scores
	for (int i = 0; i < 32; i++)
	{
		curboard->pieces [i].type = pt_none;
		curboard->pieces [i].side = (i > 15);
		curboard->pieces [i].flags = 0;
		curboard->pieces [i].square = 0;
		curboard->victim [i] = 0;
		curboard->attack [i] = 0;
	}

	curboard->attack [32] = curboard->victim [32] = 0;

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
					curboard->pieces [idx [side]].movefunc = move_pawnmove;
					curboard->victim [idx [side]] = 100;
					curboard->attack [idx [side]] = 6;
				break;
				case 'N':
					curboard->pieces [idx [side]].type = pt_knight;
					curboard->pieces [idx [side]].movefunc = move_knightmove;
					curboard->victim [idx [side]] = 200;
					curboard->attack [idx [side]] = 5;
				break;
				case 'B':
					curboard->pieces [idx [side]].type = pt_bishop;
					curboard->pieces [idx [side]].movefunc = move_bishopmove;
					curboard->victim [idx [side]] = 300;
					curboard->attack [idx [side]] = 4;
				break;
				case 'R':
					curboard->pieces [idx [side]].type = pt_rook;
					curboard->pieces [idx [side]].movefunc = move_rookmove;
					curboard->victim [idx [side]] = 500;
					curboard->attack [idx [side]] = 3;
				break;
				case 'Q':
					curboard->pieces [idx [side]].type = pt_queen;
					curboard->pieces [idx [side]].movefunc = move_queenmove;
					curboard->victim [idx [side]] = 600;
					curboard->attack [idx [side]] = 2;
				break;
				case 'K':
					curboard->pieces [idx [side]].type = pt_king;
					curboard->pieces [idx [side]].movefunc = move_kingmove;
					curboard->victim [idx [side]] = 700;
					curboard->attack [idx [side]] = 1;
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

			// add to our score
			curboard->score [side] += piecevals [curboard->pieces [idx [side]].type];
			curboard->score [side] += posvals [side] [curboard->pieces [idx [side]].type] [rank + file];

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

	it ++;

	if (*it != '-') // en passant is active
	{
		uint8 epfile = it [0] - 'a';
		uint8 eprank = it [1] - '1';

		// the rank will be off by one, since it specifies the landing square, but we don't care about that yet
		if (curboard->side)
			eprank ++; // white is vulnerable to en passant
		else
			eprank --;

		curboard->enpas = curboard->squares [21 + epfile + eprank * 10].piece;
	}

	poskey = hash_poskey ();
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
	uint8 rank = 8;
	square *sq;

	printf ("\n   a b c d e f g h\n");

	for (int i = 91; i > 20; i -= 10) // for each rank
	{
		printf ("\n%u  ", rank--);
		for (int j = 0; j < 8; j++)
		{
			uint8 side;
			sq = &curboard->squares [i + j];

			if (sq->piece && (sq->piece->flags & pf_taken) == 0)
			{
				if (sq->piece->side)
					side = 32; // 32 == 'a' - 'A'
				else
					side = 0;
	
				switch (sq->piece->type)
				{
					case pt_pawn:
						putchar ('P' + side);
					break;
					case pt_knight:
						putchar ('N' + side);
					break;
					case pt_bishop:
						putchar ('B' + side);
					break;
					case pt_rook:
						putchar ('R' + side);
					break;
					case pt_queen:
						putchar ('Q' + side);
					break;
					case pt_king:
						putchar ('K' + side);
					break;
				}
			}
			else
				putchar ('.');

			putchar (' ');
		}
	}

	putchar ('\n');
}
