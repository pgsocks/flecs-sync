#include "protocol.h"
#include <inttypes.h>

static unsigned current, count;
static bool (*query_state)(const char *);

void cucumber_queries(bool (*query)(const char *))
{
	query_state = query;
}
static int64_t actual[8192], expected[8192];
static char last[524288] = "ok operation=0 count=0";

static void input(char *line, size_t size)
{
	if (!fgets(line, (int)size, stdin)) {
		fprintf(stderr, "fixture protocol ended before teardown\n");
		exit(1);
	}
	size_t n = strlen(line);
	if (!n || line[n - 1] != '\n') {
		fprintf(stderr, "fixture command too long\n");
		exit(1);
	}
	line[n - 1] = 0;
}

void cucumber_begin(char *name, size_t size)
{
	char line[256];

	setvbuf(stdout, NULL, _IOLBF, 0);
	for (;;) {
		input(line, sizeof(line));
		if (!strncmp(line, "begin ", 6) && strlen(line + 6) < size) {
			strcpy(name, line + 6);
			puts("ok ready=1");
			return;
		}
		puts("error expected=begin");
	}
}

void cucumber_wait(unsigned operation, uint32_t signature)
{
	char line[256], wanted[64];

	snprintf(wanted, sizeof(wanted), "step %u %" PRIu32,
		operation, signature);
	for (;;) {
		input(line, sizeof(line));
		if (!strcmp(line, "state")) {
			puts(last);
			continue;
		}
		if (!strncmp(line, "query ", 6)) {
			if (!query_state || !query_state(line + 6))
				puts("error query=unknown_or_unavailable");
			continue;
		}
		if (!strcmp(line, wanted))
			break;
		if (!strcmp(line, "quit")) {
			fprintf(stderr, "fixture aborted at operation %u\n",
				operation);
			exit(1);
		}
		puts("error command=unknown_or_out_of_order");
	}
	current = operation;
	count = 0;
}

void cucumber_reply(void)
{
	size_t used = (size_t)snprintf(last, sizeof(last),
		"ok operation=%u count=%u", current, count);

	for (unsigned i = 0; i < count; i++)
		used += (size_t)snprintf(last + used, sizeof(last) - used,
			" a%u=%" PRId64 " e%u=%" PRId64,
			i, actual[i], i, expected[i]);
	puts(last);
}

void cucumber_observe(int64_t value, int64_t reference)
{
	if (count == sizeof(actual) / sizeof(*actual)) {
		fprintf(stderr, "fixture observation limit exceeded\n");
		exit(1);
	}
	actual[count] = value;
	expected[count++] = reference;
	if (value != reference) {
		cucumber_reply();
		fprintf(stderr, "operation %u: observed %" PRId64
			" expected %" PRId64 "\n", current, value, reference);
		exit(1);
	}
}

void cucumber_done(void)
{
	char line[256];

	for (;;) {
		input(line, sizeof(line));
		if (!strcmp(line, "state")) {
			puts(last);
			continue;
		}
		if (!strcmp(line, "quit")) {
			puts("ok closed=1");
			return;
		}
		puts("error expected=quit");
	}
}
