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

#ifndef BOARD_H__
#define BOARD_H__

// file enum for convenience
enum
{
	file_a,
	file_b,
	file_c,
	file_d,
	file_e,
	file_f,
	file_g,
	file_h,
};

enum { pf_moved = 1, pf_taken = 2 }; // piece flags

struct piece_s;
typedef struct square_s
{
	struct piece_s *piece;
	uint8 padding;
} square;

struct movelist_s;
typedef struct piece_s
{
	uint8 square;
	enum { ps_white, ps_black } side;
	uint8 flags;
	enum
	{
		pt_none,
		pt_pawn,
		pt_knight,
		pt_bishop,
		pt_rook,
		pt_queen,
		pt_king
	} type;
	struct movelist_s *(*movefunc) (uint8 piece);
} piece;

typedef struct board_s
{
	square squares [120];
	piece pieces [32]; // 0-15 white, 16-32 black
	enum { bs_white, bs_black } side; // side to play

	uint8 cast [2] [2];

	piece *rooks [2] [2];
	piece *kings [2];
	piece *enpas;

	int16 score [2];
} board;

extern board *curboard;

uint8 board_getsquare (int8 file, int8 rank);
uint8 board_piecesquare (uint8 index);
uint8 board_getfile (uint8 square);
uint8 board_getrank (uint8 square);
void board_initialize (const char *fen);
uint8 board_squareattacked (uint8 sq);
void board_print (void);

#endif
