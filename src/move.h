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

#ifndef MOVE_H__
#define MOVE_H__

enum
{
	ms_null,
	ms_firstmove,
	ms_enpas,
	ms_enpascap,
	ms_kcast,
	ms_qcast,
	ms_qpromo,
	ms_rpromo,
	ms_bpromo,
	ms_npromo
};

typedef struct move_s
{
	uint8 piece;
	uint8 taken;
	uint8 square;
	uint8 from;
	uint8 special;

	uint32 score;
} move;

typedef struct movelist_s
{
	move m;

	struct movelist_s *next;
} movelist;

extern movelist *moveroot;
extern move history [128];
extern uint8 htop;

void move_apply (move *m);
void move_make (move *m);
void move_undo (move *m);
void move_initnodes (void);
movelist *move_newnode (uint8 piece, uint8 taken, uint8 square, uint8 from);
void move_clearnodes (movelist *m);
movelist *move_genlist (void);
void move_print (move *m, char *c);
void move_decode (const char *c, move *m);

movelist *move_pawnmove (uint8 piece, movelist **tail);
movelist *move_knightmove (uint8 piece, movelist **tail);
movelist *move_bishopmove (uint8 piece, movelist **tail);
movelist *move_rookmove (uint8 piece, movelist **tail);
movelist *move_queenmove (uint8 piece, movelist **tail);
movelist *move_kingmove (uint8 piece, movelist **tail);

#endif
