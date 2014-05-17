#include "int.h"
#include "board.h"

int16 piecevals [7] = { 0, 100, 300, 310, 500, 900, 10000 };
void board_print (void);

int16 evaluate (void)
{
	int16 wmaterial = 0, bmaterial = 0, *material;
	int i;

//	board_print ();

	for (i = 0; i < 32; i++)
	{
		if (i < 16)
			material = &wmaterial;
		else
			material = &bmaterial;

		if (curboard->pieces [i] & pf_taken)
			continue;

		switch (curboard->pieces [i] & 0x07)
		{
			case pt_pawn:
				*material += piecevals [pt_pawn];
			break;
			case pt_knight:
				*material += piecevals [pt_knight];
			break;
			case pt_bishop:
				*material += piecevals [pt_bishop];
			break;
			case pt_rook:
				*material += piecevals [pt_rook];
			break;
			case pt_queen:
				*material += piecevals [pt_queen];
			break;
			case pt_king:
				*material += piecevals [pt_king];
			break;
		}
	}

	return curboard->flags & bf_side ? bmaterial - wmaterial : wmaterial - bmaterial;
}
