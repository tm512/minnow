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

#ifndef HASH_H__
#define HASH_H__

enum { et_null, et_exact, et_alpha, et_beta };

typedef struct
{
	uint64 key;
	uint8 depth;
	uint8 type;
	int16 score;
	move best;
} hashentry;

extern uint64 poskey;
extern uint64 keytable [1680];
extern uint64 sidekey;
extern uint64 castkeys [4];
extern uint64 epkeys [8];
extern uint64 promokeys [32][4];

void hash_init (uint64 bytes);
void hash_clear (void);
uint64 hash_poskey (void);
int16 hash_probe (uint8 depth, int16 alpha, int16 beta, uint8 *type, move **best);
void hash_store (uint8 depth, int16 score, uint8 type, move *best);
void hash_info (void);

#define hash_pieceidx(i) (((curboard->pieces [i].type << 1) + curboard->pieces [i].side) * 120)

#endif
