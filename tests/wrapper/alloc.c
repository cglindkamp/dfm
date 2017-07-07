#include <stdbool.h>
#include <stdlib.h>

#include "alloc.h"

void *__real_malloc(size_t size);
void *__real_realloc(void *ptr, size_t size);
char *__real_strdup(const char *s);

static int working_allocations = -1;

void alloc_set_allocations_until_fail(int allocations)
{
	working_allocations = allocations;
}

void *__wrap_malloc(size_t size) {
	if(working_allocations == 0)
		return 0;
	if(working_allocations > 0)
		working_allocations--;
	return __real_malloc(size);
}

void *__wrap_realloc(void *ptr, size_t size) {
	if(working_allocations == 0)
		return 0;
	if(working_allocations > 0)
		working_allocations--;
	return __real_realloc(ptr, size);
}

char *__wrap_strdup(const char *s) {
	if(working_allocations == 0)
		return 0;
	if(working_allocations > 0)
		working_allocations--;
	return __real_strdup(s);
}

