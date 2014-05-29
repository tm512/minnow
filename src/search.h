#ifndef SEARCH_H__
#define SEARCH_H__

void search (uint8 depth, move *best);
int16 absearch (uint8 depth, move *best, int16 alpha, int16 beta);
uint64 perft (uint8 depth, uint8 start);

#endif
