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

movelist *move_pawnmove (uint8 piece);
movelist *move_knightmove (uint8 piece);
movelist *move_bishopmove (uint8 piece);
movelist *move_rookmove (uint8 piece);
movelist *move_queenmove (uint8 piece);
movelist *move_kingmove (uint8 piece);

#endif
