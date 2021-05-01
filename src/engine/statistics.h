#ifndef __STATISTICS_H
#define __STATISTICS_H

#include "types.h"

struct context;
struct memory;

typedef i32 statistics_timestamp_id;

#define STATISTICS_MAX_TIMESTAMP_LABEL_LENGTH 12
#define STATISTICS_MAX_TIMESTAMP_DESC_LENGTH 32
#define STATISTICS_MAX_TIMESTAMPS 10

struct statistics_timestamp
{
	u8 _header[sizeof(u32) + sizeof(b32)];

	statistics_timestamp_id id;
	u8 name[STATISTICS_MAX_TIMESTAMP_LABEL_LENGTH];
	u8 desc[STATISTICS_MAX_TIMESTAMP_DESC_LENGTH];

	u64 ticksSinceStart;
	
	struct context *app;
};

struct statistics
{
	void *_userPtr;
	struct statistics_timestamp _timeStamps[STATISTICS_MAX_TIMESTAMPS];
};

struct statistics *
statistics_init(struct context *app);

#endif
