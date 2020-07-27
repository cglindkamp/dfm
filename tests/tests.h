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
			ck_assert_int_eq(write(oom_pipe[1], &(char){' '}, 1), 1); \
			__VA_ARGS__; \
			return; \
		} \
	} else { \
		ck_assert(condition); \
	}

#define assert_oom(condition) assert_oom_cleanup(condition,)

#endif
