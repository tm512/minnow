#include <stdio.h>

#include "int.h"
#include "board.h"
#include "move.h"

void board_print (void);

int16 evaluate (void)
{
	return curboard->score [curboard->side] - curboard->score [!curboard->side];
}
