#ifdef _WIN32
	#include <windows.h>
	#include <winbase.h>
#else
	#define _POSIX_C_SOURCE 199309L
	#include <stdio.h>
	#include <unistd.h>
	#ifdef __FreeBSD__
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
