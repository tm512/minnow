#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

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

inline uint8 board_piecesquare (uint8 index)
{
	return 0;
}

#if 0
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
#else


void board_initialize (const char *fen)
{
	int i;
	uint8 rank = 91, file = 0;
	const char *it = fen;
	// track indexes into the piece array
	uint8 idx [2] = { 0, 16 };

	if (!curboard)
		curboard = malloc (sizeof (board));

	// initialize castling stuff
	curboard->cast [0] [0] = curboard->cast [0] [1] = 0;
	curboard->cast [1] [0] = curboard->cast [1] [1] = 0;
	curboard->rooks [0] [0] = curboard->rooks [0] [1] = NULL;
	curboard->rooks [1] [0] = curboard->rooks [1] [1] = NULL;

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

			// we need to track kings and rooks for castling and mating
			if (p == 'K')
				curboard->kings [side] = &curboard->pieces [idx [side]];

			if (p == 'R')
			{
				if (!curboard->rooks [side] [0])
					curboard->rooks [side] [0] = &curboard->pieces [idx [side]];
				else
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
	printf ("%s\n", it);

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
#endif

uint8 board_squareattacked (uint8 sq)
{
	movelist *m = move_genlist (), *it;
	it = m;

	while (it)
	{
		if (it->m.square == sq)
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

//const char *startfen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
//const char *startfen = "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -";
const char *startfen = "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQ -";

extern uint64 numnodes;
int main (void)
{
	int i = 0;
	move best;
	board_initialize (startfen);
	board_print ();
/*
	while ((curboard->pieces [15].flags & pf_taken) == 0 && (curboard->pieces [31].flags & pf_taken) == 0)
	{
		clock_t start = clock ();
		absearch (5, &best, -30000, 30000);
		printf ("search took %f seconds\n", (float)(clock () - start) / (float)CLOCKS_PER_SEC);

		move_make (&best);
		board_print ();
//		printf ("%u nodes stored (%fMiB)\n", numnodes,
//		        (float)(numnodes * (sizeof (movelist) + sizeof (move))) / 1048576.0f);
		getchar ();
	}
*/

	clock_t start = clock ();
	uint64 nodes = perft (2, 2);
	printf ("perft: %u nodes, %f seconds\n", nodes, (float)(clock () - start) / (float)CLOCKS_PER_SEC);

	return 0;
}
