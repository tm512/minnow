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

#include "int.h"
#include "board.h"
#include "move.h"

int16 evaluate (int16 alpha, int16 beta)
{
	int i;
	int32 ret [2] = { curboard->score [0], curboard->score [1] };
	int32 base, ratio;
	uint8 knights [2] = { 0 }, bishops [2] = { 0 }, rooks [2] = { 0 };

	// fixed point grossness:
	ratio = (ret [curboard->side] << 6) / ret [!curboard->side];
	base = (((ret [curboard->side] - ret [!curboard->side]) << 6) * ratio) >> 12;

	if (base >= beta + (((50 << 6) * ratio) >> 12))
		return base;

	if (base < alpha - (((50 << 6) * ratio) >> 12))
		return base;

	// check for piece pairs
	for (i = 0; i < 32; i++)
	{
		if (curboard->pieces [i].flags & pf_taken || curboard->pieces [i].type == pt_none)
			continue;

		switch (curboard->pieces [i].type)
		{
			case pt_knight:
				knights [(i > 15)] ++;
			break;
			case pt_bishop:
				bishops [(i > 15)] ++;
			break;
			case pt_rook:
				rooks [(i > 15)] ++;
			break;
		}
	}

	for (i = 0; i < 2; i++)
	{
		if (knights [i] == 2)
			ret [i] -= 5;

		if (bishops [i] == 2)
			ret [i] += 20;

		if (rooks [i] == 2)
			ret [i] -= 10;
	}

	return (int16)((((ret [curboard->side] - ret [!curboard->side]) << 6) * ((ret [curboard->side] << 6) / ret [!curboard->side])) >> 12);
}
