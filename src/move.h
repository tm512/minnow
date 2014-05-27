#ifndef MOVE_H__
#define MOVE_H__

enum
{
	ms_null,
	ms_firstmove,
	ms_enpas,
	ms_enpascap,
	ms_kcast,
	ms_qcast
};

typedef struct move_s
{
	uint8 piece;
	uint8 taken;
	uint8 square;
	uint8 from;
	uint8 special;

	// for game history
	struct move_s *prev;
} move;

typedef struct movelist_s
{
	move *m;

	struct movelist_s *parent;
	struct movelist_s *next;
	struct movelist_s *child;
} movelist;

extern move *history;
extern movelist *moveroot;

void move_apply (move *m);
void move_make (movelist *m);
void move_undo (move *m);
movelist *move_newnode (movelist *parent);
void move_clearnodes (movelist *m);
void move_genlist (movelist *start);

movelist *move_pawnmove (movelist *parent, uint8 piece);
movelist *move_knightmove (movelist *parent, uint8 piece);
movelist *move_bishopmove (movelist *parent, uint8 piece);
movelist *move_rookmove (movelist *parent, uint8 piece);
movelist *move_queenmove (movelist *parent, uint8 piece);
movelist *move_kingmove (movelist *parent, uint8 piece);

#endif
