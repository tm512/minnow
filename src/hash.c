/* Copyright (c) 2014-2015 Kyle Davis, All Rights Reserved.

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

#define HASHDEBUG 0
#define NOHASHING 0

uint64 prngstate;
uint64 poskey = 0;

uint64 keytable [1680];
uint64 sidekey;
uint64 castkeys [4];
uint64 epkeys [8];
uint64 promokeys [32][4];

uint64 entries;
hashentry *hashtable = NULL;

#if HASHDEBUG
uint64 *collisiontable = NULL;
uint64 totalstores = 0;
#endif

uint64 xorshift (uint64 x)
{
	x ^= x << 13;
	x ^= x >> 7;
	x ^= x << 17;
	return x * 0x2545f4914f6cdd1d;
}

// splitmix64 implementation from duktape
uint64_t splitmix64 (uint64_t x)
{
	x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9;
	x = (x ^ (x >> 27)) * 0x94d049bb133111eb;
	return x ^ (x >> 31);
}

uint64 prng (void)
{
	prngstate += 0x9e3779b97f4a7c15;
	return splitmix64 (prngstate);
}

void hash_init (uint64 bytes)
{
	prngstate = 0xac2c986921b37e05;

	for (int i = 0; i < 1680; i++)
		keytable [i] = prng ();

	for (int i = 0; i < 1680; i++)
	for (int j = 0; j < 1680; j++)
		if (i != j && keytable [i] == keytable [j])
			printf ("keytable collision\n");

	sidekey = prng ();

	for (int i = 0; i < 4; i++)
		castkeys [i] = prng ();

	for (int i = 0; i < 8; i++)
		epkeys [i] = prng ();

	for (int i = 0; i < 32; i++)
	for (int j = 0; j < 4; j++)
		promokeys [i] [j] = prng ();

	#if !NOHASHING
	entries = bytes / sizeof (hashentry);

	free (hashtable);

	if (bytes == 0)
		entries = 1;

	hashtable = malloc (entries * sizeof (hashentry));
	hash_clear ();

	#if HASHDEBUG
	free (collisiontable);
	collisiontable = malloc (entries * sizeof (uint64));
	for (int i = 0; i < entries; i++)
		collisiontable [i] = 0;

	totalstores = 0;
	#endif

	printf ("hash table initialized with %u entries\n", entries);
	#else
	printf ("hash table disabled at compile-time\n");
	#endif
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

#define currentside (curboard->side ? "black" : "white")
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

		#if HASHDEBUG
		printf ("poskey: %c%i: %s %s (%16llX)\n",
		        'a' + board_getfile (curboard->pieces [i].square),
		        1 + board_getrank (curboard->pieces [i].square),
		        currentside, board_piecetype (&curboard->pieces [i]),
		        keytable [hash_pieceidx (i) + curboard->pieces [i].square]);
		#endif

		ret ^= keytable [hash_pieceidx (i) + curboard->pieces [i].square];
	}

	#if HASHDEBUG
	printf ("poskey: %s to move (%16llX)\n", currentside, curboard->side ? sidekey : 0);
	#endif
	if (curboard->side)
		ret ^= sidekey;

	if (curboard->cast [0] [0])
	{
		#if HASHDEBUG
		printf ("poskey: white queenside castling (%16llX)\n", castkeys [0]);
		#endif
		ret ^= castkeys [0];
	}

	if (curboard->cast [0] [1])
	{
		#if HASHDEBUG
		printf ("poskey: white kingside castling (%16llX)\n", castkeys [1]);
		#endif
		ret ^= castkeys [1];
	}

	if (curboard->cast [1] [0])
	{
		#if HASHDEBUG
		printf ("poskey: black queenside castling (%16llX)\n", castkeys [2]);
		#endif
		ret ^= castkeys [2];
	}

	if (curboard->cast [1] [1])
	{
		#if HASHDEBUG
		printf ("poskey: black kingside castling (%16llX)\n", castkeys [3]);
		#endif
		ret ^= castkeys [3];
	}

	if (curboard->enpas)
	{
		#if HASHDEBUG
		printf ("poskey: %c file en passant (%16llX)\n", 'a' + board_getfile (curboard->enpas->square),
		        epkeys [board_getfile (curboard->enpas->square)]);
		#endif
		ret ^= epkeys [board_getfile (curboard->enpas->square)];
	}

	return ret;
}

static inline uint64_t fastidx (uint64_t key)
{
	return ((key & 0xffffffff) * entries) >> 32;
}

int16 hash_probe (uint8 depth, int16 alpha, int16 beta, move **best)
{
	#if !NOHASHING
	hashentry *e = &hashtable [fastidx (poskey)];

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
		}

		// prioritize this move in the search
		if (e->best.square != 0)
			*best = &e->best;
	}
	#endif

	return -32000;
}

void hash_store (uint8 depth, int16 score, uint8 type, move *best)
{
	#if !NOHASHING
	hashentry *e = &hashtable [fastidx (poskey)];

	#if HASHDEBUG
	totalstores ++;
	if (e->type != et_null && e->key != poskey)
		collisiontable [poskey % entries]++;
	#endif

	e->key = poskey;
	e->depth = depth;
	e->score = score;
	e->type = type;

	if (best)
		e->best = *best;
	else
		e->best.square = 0;
	#endif
}

void hash_info (void)
{
	#if NOHASHING
	printf ("hash table disabled\n");
	return;
	#endif

	uint64 occupied = 0;
	for (int i = 0; i < entries; i++)
		occupied += (hashtable [i].type != et_null);

	#if HASHDEBUG
	uint64 totalcoll = 0;
	struct { int idx; uint64 count; } top [16] = { 0 };
	for (int i = 0; i < entries; i++)
	{
		totalcoll += collisiontable [i];

		for (int j = 0; j < 16; j++)
		{
			if (collisiontable [i] > top [j].count)
			{
				top [j].idx = i;
				top [j].count = collisiontable [i];
				break;
			}
		}
	}

	printf ("%llu collisions total:\n", totalcoll);
	for (int i = 0; i < 16; i++)
		printf ("index %i: %llu\n", top [i].idx, top [i].count);
	#endif

	printf ("%llu/%llu entries occupied (%g%%)\n", occupied, entries, (occupied / (double)entries) * 100.);

	#if HASHDEBUG
	printf ("%llu total entries stored\n", totalstores);
	#endif
}
