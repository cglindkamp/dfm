#ifndef TEST_H
#define TEST_H

#include <stdbool.h>
#include <unistd.h>

#include "wrapper/alloc.h"

extern bool mode_oom;
extern int oom_pipe[2];


#define assert_oom_cleanup(condition, ...) \
	if(mode_oom) { \
		if(!(condition)) { \
			write(oom_pipe[1], &(char){' '}, 1); \
			alloc_set_allocations_until_fail(-1); \
			__VA_ARGS__; \
			return; \
		} \
	} else { \
		ck_assert(condition); \
	}

#define assert_oom(condition) assert_oom_cleanup(condition,)

#undef END_TEST
#define END_TEST \
		alloc_set_allocations_until_fail(-1); \
	} \

#endif
