#include <stdio.h>

#include "int.h"
#include "board.h"
#include "move.h"
#include "posvals.h"

int16 piecevals [7] = { 0, 100, 300, 310, 500, 900, 10000 };
void board_print (void);

int16 evaluate (void)
{
	int16 wmaterial = 0, bmaterial = 0;
	int i;

//	board_print ();

	for (i = 0; i < 32; i++)
	{
		if (curboard->pieces [i].flags & pf_taken)
			continue;

		if (i < 16)
		{
			wmaterial += piecevals [curboard->pieces [i].type];
			wmaterial += posvals [bs_white] [curboard->pieces [i].type] [curboard->pieces [i].square];
		}
		else
		{
			bmaterial += piecevals [curboard->pieces [i].type];
			bmaterial += posvals [bs_black] [curboard->pieces [i].type] [curboard->pieces [i].square];
		}

		//*material += posvals [curboard->pieces [i].type] [curboard->pieces [i].side] [curboard->pieces [i].square];
	}

	return curboard->side ? bmaterial - wmaterial : wmaterial - bmaterial;
}
