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
#include <assert.h>

#include "int.h"
#include "board.h"
#include "move.h"
#include "hash.h"

uint64 xorstate;
uint64 poskey = 0;

uint64 keytable [3840];
uint64 sidekey;
uint64 castkeys [4];
uint64 epkeys [8];
uint64 promokeys [32][4];

uint64 entries;
hashentry *hashtable = NULL;

uint64 xor (void)
{
	xorstate ^= xorstate << 13;
	xorstate ^= xorstate >> 7;
	xorstate ^= xorstate << 17;
	return xorstate;
}

void hash_init (uint64 bytes)
{
	xorstate = 0xac2c986921b37e05;

	for (int i = 0; i < 3840; i++)
		keytable [i] = xor ();

	for (int i = 0; i < 3840; i++)
	for (int j = 0; j < 3840; j++)
		if (i != j && keytable [i] == keytable [j])
			printf ("keytable collision\n");

	sidekey = xor ();

	for (int i = 0; i < 4; i++)
		castkeys [i] = xor ();

	for (int i = 0; i < 8; i++)
		epkeys [i] = xor ();

	for (int i = 0; i < 32; i++)
	for (int j = 0; j < 4; j++)
		promokeys [i] [j] = xor ();

	entries = bytes / sizeof (hashentry);

	free (hashtable);

	if (bytes == 0)
		entries = 1;

	hashtable = malloc (entries * sizeof (hashentry));
	hash_clear ();

	printf ("hash table initialized with %u entries\n", entries);
}

void hash_clear (void)
{
	for (uint64 i = 0; i < entries; i++)
	{
		hashtable [i].key = 0;
		hashtable [i].type = et_null;
		hashtable [i].best.square = 0;
	}
}

uint8 poshack = 0;
uint64 hash_poskey (void)
{
	uint64 ret = 0;
	uint32 idx;

	// for each piece, xor its unique ID into ret
	for (int i = 0; i < 32; i++)
	{
		if (curboard->pieces [i].flags & pf_taken || curboard->pieces [i].type == pt_none)
			continue;

		ret ^= keytable [(i * 120) + curboard->pieces [i].square];
	}

	if (curboard->side)
		ret ^= sidekey;

	if (curboard->cast [0] [0])
		ret ^= castkeys [0];

	if (curboard->cast [0] [1])
		ret ^= castkeys [1];

	if (curboard->cast [1] [0])
		ret ^= castkeys [2];

	if (curboard->cast [1] [1])
		ret ^= castkeys [3];

	if (curboard->enpas)
		ret ^= epkeys [board_getfile (curboard->enpas->square)];

	return ret;
}

int16 hash_probe (uint8 depth, int16 alpha, int16 beta, move **best)
{
//	uint64 poskey = hash_poskey ();
	hashentry *e = &hashtable [poskey % entries];

	if (e->key == poskey) // this entry (probably) came from the same position
	{
		if (e->depth >= depth) // shallower results aren't to be trusted
		{
			if (e->type == et_exact)
				return e->score;
			else if (e->type == et_alpha && e->score <= alpha)
				return alpha;
			else if (e->type == et_beta && e->score >= beta)
				return beta;
			else if (e->type == et_mate)
				return 15000 + depth;
		}

		if (e->best.square != 0)
			*best = &e->best;
	}

	return -32000;
}

void hash_store (uint8 depth, int16 score, uint8 type, move *best)
{
//	uint64 poskey = hash_poskey ();
	hashentry *e = &hashtable [poskey % entries];

	e->key = poskey;
	e->depth = depth;
	e->score = score;
	e->type = type;

	if (best)
		e->best = *best;
	else
		e->best.square = 0;
}
