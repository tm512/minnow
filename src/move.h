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
} move;

typedef struct movelist_s
{
	move m;

	struct movelist_s *next;
	struct movelist_s *child;
} movelist;

extern movelist *moveroot;

void move_apply (move *m);
void move_make (movelist *m);
void move_undo (move *m);
movelist *move_newnode (uint8 piece, uint8 taken, uint8 square, uint8 from);
void move_clearnodes (movelist *m);
void move_genlist (movelist *start);

movelist *move_pawnmove (uint8 piece);
movelist *move_knightmove (uint8 piece);
movelist *move_bishopmove (uint8 piece);
movelist *move_rookmove (uint8 piece);
movelist *move_queenmove (uint8 piece);
movelist *move_kingmove (uint8 piece);

#endif
