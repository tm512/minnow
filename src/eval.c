/* Copyright (c) 2014-2022 Kyle Davis, All Rights Reserved.

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

#pragma GCC optimize ("unroll-loops")

int16 contempt = -100;

int16 evaluate (int16 alpha, int16 beta)
{
	int i;
	int32 ret [2] = { curboard->matscore [0] + curboard->posscore [0], curboard->matscore [1] + curboard->posscore [1] };
	int32 base;

	// detect draw-ish situations
	// if we're ahead of the opponent and have only a king and a minor piece, the position is almost guaranteed to draw
	if (curboard->piececount [curboard->side] [pt_pawn] == 0 &&
	    curboard->matscore [curboard->side] > curboard->matscore [!curboard->side] &&
	    curboard->matscore [curboard->side] - curboard->matscore [!curboard->side] < 10400)
		return contempt;

	base = ret [curboard->side] - ret [!curboard->side];

	// lazy evaluation, just return here if the best/worst we're gonna do is still not good enough/too good
	if (base - 100 >= beta)
		return base;

	if (base + 100 < alpha)
		return base;

	for (i = 0; i < 2; i++)
	{
		if (curboard->piececount [i] [pt_knight] == 2)
			ret [i] -= 5;

		if (curboard->piececount [i] [pt_bishop] == 2)
			ret [i] += 20;

		if (curboard->piececount [i] [pt_rook] == 2)
			ret [i] -= 10;

		// we've been tracking the number of trades, and the winning side gets a small bonus for trading down
		if (curboard->matscore [i] > curboard->matscore [!i])
			ret [i] += 20 * curboard->trades;
	}

	return ret [curboard->side] - ret [!curboard->side];
}
