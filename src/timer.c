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

#ifdef _WIN32
	#include <windows.h>
	#include <winbase.h>
#else
	#define _POSIX_C_SOURCE 199309L
	#include <stdio.h>
	#include <unistd.h>
	#if defined(__FreeBSD__) || defined(__DragonFly__)
		#include <sys/time.h>
	#else
		#include <time.h>
	#endif
#endif

#include "int.h"

uint64 time_get (void)
{
	#ifdef _WIN32
	return (uint64)GetTickCount ();
	#else
	struct timespec ts;

	clock_gettime (CLOCK_MONOTONIC, &ts);

	return ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
	#endif
}

uint64 time_since (uint64 start)
{
	return time_get () - start;
}

float time_since_sec (uint64 start)
{
	return (float)time_since (start) / 1000.0f;
}

static uint64 maxtimes [6][2] =
{
	{ 300000, 5000 },
	{ 600000, 10000 },
	{ 900000, 15000 },
	{ 1800000, 30000 },
	{ 3600000, 60000 },
	{ 7200000, 120000 }
};

#define min(a, b) ((a < b) ? a : b)
uint64 time_alloc (uint64 time, uint64 nottime)
{
	uint64 ret;

	// find an upper bound based on our time left
	for (int i = 0; i < 6; i++)
		if (time < maxtimes [i] [0])
		{
			ret = maxtimes [i] [1];
			break;
		}

	// if we're running out of time, move faster
	ret = min (ret, time / 10);

	if (time - ret > nottime) // if we'd still have more time than our opponent after this move, give some extra
		ret += (time - nottime - ret) / 2;

	return ret;
}
