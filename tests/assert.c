#include "test.h"

_Noreturn void test_fail(const char *file, int line, const char *expression)
{
	fprintf(stderr, "%s:%d: %s\n", file, line, expression);
	exit(EXIT_FAILURE);
}

