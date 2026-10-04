#include "test.h"
#ifdef FILE_SYNC_TESTING
#include "private.h"
#endif

struct test_case {
	const char *name, *example;
	bool (*run)(struct rpg_context *, const char *, char *, size_t);
};
static const struct test_case cases[] = {
#define CASE(id, example, function) {#id, example, function},
#include "cases.def"
#undef CASE
};

int main(int argc, char **argv)
{
	if (argc != 2) {
		fprintf(stderr, "Usage: %s CASE\n", argv[0]);
		return 1;
	}
	const struct test_case *selected = NULL;
	for (size_t i = 0; i < sizeof(cases) / sizeof(*cases); i++)
		if (!strcmp(argv[1], cases[i].name)) {
			selected = &cases[i];
			break;
		}
	if (!selected) {
		fprintf(stderr, "Unknown RPG case: %s\n", argv[1]);
		return 1;
	}
	struct rpg_context ctx = {0};
	char error[1024] = {0};
#ifdef FILE_SYNC_TESTING
	file_sync_test_clear();
#endif
	bool passed = rpg_initialize(&ctx);
	if (passed)
		passed = selected->run(
			&ctx, selected->example, error, sizeof(error));
	else
		snprintf(error, sizeof(error),
			"RPG fixture initialization failed");
#ifdef FILE_SYNC_TESTING
	file_sync_test_clear();
#endif
	if (!rpg_destroy(&ctx)) {
		fprintf(stderr, "%s: fixture cleanup failed\n", selected->name);
		passed = false;
	}
	if (!passed) {
		fprintf(stderr, "FAIL %s: %s\n", selected->name, error);
		return 1;
	}
	printf("PASS %s\n", selected->name);
	return 0;
}
