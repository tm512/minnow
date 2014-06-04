#include <stdio.h>

#include "int.h"
#include "board.h"
#include "move.h"

void board_print (void);

int16 evaluate (void)
{
	return curboard->side ? curboard->score [1] - curboard->score [0] : curboard->score [0] - curboard->score [1];
}
