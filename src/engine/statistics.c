#include "statistics.h"
#include "engine.h"
#include "memory.h"

#include <stdio.h>

struct statistics
{
	const struct memory_allocation_key *userPtr;
	struct statistics_timestamp _timeStamps[STATISTICS_MAX_TIMESTAMPS];
	const struct context *app;
};

b32
statistics_init(const struct context *app, const struct memory_context_key *memoryKeyPtr)
{
	{
		b32 isArgsSafe = B32_TRUE;
		
		if (!app)
		{
			fprintf(stderr, "%s(%d): 'app' argument cannot be NULL. Failure to allocate statistics.\n", 
					__FUNCTION__, __LINE__);
			
			isArgsSafe = B32_FALSE;
		}

		if (!memoryKeyPtr)
		{
			fprintf(stderr, "%s(%d): 'app' argument cannot be NULL. Failure to allocate statistics.\n", 
					__FUNCTION__, __LINE__);
			
			isArgsSafe = B32_FALSE;
		}

		if (!isArgsSafe)
		{
			return B32_FALSE;
		}
	}

	// TODO: implement!

	return B32_FALSE;
}