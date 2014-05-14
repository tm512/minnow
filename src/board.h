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

// piece types
enum
{
	pt_empty,
	pt_pawn,
	pt_knight,
	pt_bishop,
	pt_rook,
	pt_queen,
	pt_king,
};

// board flags
enum
{
	bf_side, // 0 white to play, 1 black to play
};

// square flags
enum
{
	sf_wocc = 32, // occupied by white
	sf_bocc = 64, // occupied by black
	sf_padding = 128
};

// piece flags
enum
{
	pf_moved = 1024,
	pf_taken = 2048
};

typedef struct board_s
{
	uint8 flags;
	uint8 squares [120];
	uint16 pieces [32]; // 0-15 white, 16-32 black

	// for searching
	struct board_s *parent;
	struct board_s *children;
	struct board_s *next;
} board;

extern board *curboard;

uint8 board_getsquare (int8 file, int8 rank);
uint8 board_piecesquare (uint8 index);
uint8 board_getfile (uint8 square);
uint8 board_getrank (uint8 square);

#endif
