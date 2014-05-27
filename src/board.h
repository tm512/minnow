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
} piece;

typedef struct board_s
{
	square squares [120];
	piece pieces [32]; // 0-15 white, 16-32 black
	enum { bs_white, bs_black } side; // side to play
} board;

extern board *curboard;

uint8 board_getsquare (int8 file, int8 rank);
uint8 board_piecesquare (uint8 index);
uint8 board_getfile (uint8 square);
uint8 board_getrank (uint8 square);
uint8 board_squareattacked (uint8 sq);

#endif
