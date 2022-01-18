#include <argp.h>
#include "common.hpp"
#include "compiler.hpp"

// ================= arg stuff =======================

const char *argp_program_version = APP_NAME " " APP_VERSION;
const char *argp_program_bug_address = EMAIL;

#define ARGS_COUNT 1

/* This structure is used by main to communicate with parse_opt. */
struct arguments
{
	char *args[ARGS_COUNT];	/* script */
	int verbose;			/* The -v flag */
	char *outfile;

	bool output_given;
};

static struct argp_option options[] =
{
	{"verbose", 'v', 0, 0, "Produce verbose output"},
	{"output",  'o', "OUTFILE", 0, "Output to OUTFILE instead of to standard output"},
	{0}
};

static error_t parse_opt(int key, char *arg, struct argp_state *state)
{
	struct arguments *arguments = (struct arguments*)state->input;

	switch (key)
	{
	case 'v':
		arguments->verbose += 1;
		break;
	case 'o':
		arguments->outfile = arg;
		arguments->output_given = true;
		break;


	case ARGP_KEY_ARG:
	{
		if (state->arg_num >= ARGS_COUNT)
		{
			argp_usage(state);
		}
		arguments->args[state->arg_num] = arg;
		break;
	}
	case ARGP_KEY_END:
	{
		if (state->arg_num < 1 || state->arg_num < ARGS_COUNT)
		{
			argp_usage(state);
		}
		break;
	}
	default: return ARGP_ERR_UNKNOWN;
	}
	return 0;
}

static char args_doc[] = "[file]";
static char *doc = strdup(tools::fstr(
	APP_DOC, APP_NAME, EMAIL, LINK, __DATE__, __TIME__, OS_NAME, COMPILER
	).c_str());
static struct argp argp = {options, parse_opt, args_doc, doc};

// ================================

int main(int argc, char **argv)
{
	// ========= argument stuff =========

	struct arguments arguments;

	/* Set argument defaults */
	arguments.verbose = false;
	arguments.output_given = false;

	/* Where the magic happens */
	argp_parse(&argp, argc, argv, 0, 0, &arguments);

	// figure out output file name
	if(!arguments.output_given)
	{
		arguments.outfile = strdup(arguments.args[0]);
		arguments.outfile = strtok(arguments.outfile, ".");
	}

	printf("file: '%s', output: '%s', verbose: %d\n",
		arguments.args[0], arguments.outfile, arguments.verbose);

	// ===================================

	Compiler compiler;
	compiler.configure(arguments.args[0], arguments.outfile);
	compiler.compile();

	return 0;
}