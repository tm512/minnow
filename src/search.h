#ifndef SEARCH_H__
#define SEARCH_H__

// Idea from Bruce Moreland's Computer Chess Pages
typedef struct pvlist_s
{
	uint8 nodes;
	move moves [64];
} pvlist;

int16 search (uint8 depth, uint64 maxtime, move *best);
int16 absearch (uint8 depth, uint8 start, pvlist *pv, pvlist *oldpv, int16 alpha, int16 beta);
uint64 perft (uint8 depth, uint8 start);

#endif
