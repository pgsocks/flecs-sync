#include "test.h"

#define CASE(name) void name(void);
#include "cases.def"
#undef CASE

static const struct test_case {
	const char *name;
	void (*run)(void);
} cases[] = {
#define CASE(name) { #name, name },
#include "cases.def"
#undef CASE
};

int main(int argc, char **argv)
{
	if (argc != 2) {
		fprintf(stderr, "Usage: %s CASE\n", argv[0]);
		return EXIT_FAILURE;
	}
	for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
		if (strcmp(argv[1], cases[i].name))
			continue;
		fprintf(stderr, "CASE: %s\n", cases[i].name);
		cases[i].run();
		fprintf(stderr, "PASS: %s\n", cases[i].name);
		return EXIT_SUCCESS;
	}
	fprintf(stderr, "Unknown case: %s\n", argv[1]);
	return EXIT_FAILURE;
}
