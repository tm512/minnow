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

		if (curboard->pieces [i].flags & pf_taken)
			continue;

		*material += piecevals [curboard->pieces [i].type];
	}

//	printf ("evaluating as %i\n", curboard->side);
	return curboard->side ? bmaterial - wmaterial : wmaterial - bmaterial;
}
