#ifndef TIMER_H__
#define TIMER_H__

uint64 time_get (void);
uint64 time_since (uint64 start);
float time_since_sec (uint64 start);
uint64 time_alloc (uint64 time, uint64 nottime);

#endif
