#define ARGP_NO_EXIT
#include <argp.h>
#include "common.hpp"
#include "parser.hpp"
#include "typechecker.hpp"
#include "codegen.hpp"

#ifdef DEBUG
#include "debug.hpp"
#endif

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
	if(argp_parse(&argp, argc, argv, 0, 0, &arguments)) return STATUS_CLI_ERROR;

	// figure out output file name
	if(!arguments.output_given)
	{
		arguments.outfile = strdup(arguments.args[0]);
		arguments.outfile = strtok(arguments.outfile, ".");
	}

	// ===================================

	AST astree;
	const char* source = strdup(tools::readf(arguments.args[0]).c_str());

	init_builtin_evi_types();

	// parse program
	Parser parser;
	Status parser_status = parser.parse(arguments.args[0], source, &astree);
	if(parser_status != STATUS_SUCCESS) { free((void*)source); return parser_status; }

	
	// generate visualization
	#ifdef DEBUG
	ASTVisualizer().visualize(string(arguments.args[0]) + ".svg", &astree);
	#endif

	
	// type check
	TypeChecker checker;
	Status checker_status = checker.check(arguments.args[0], source, &astree);
	if(checker_status != STATUS_SUCCESS) { free((void*)source); return checker_status; }


	// codegen
	CodeGenerator codegen;
	Status codegen_status = codegen.generate(arguments.outfile, source, &astree);
	if(codegen_status != STATUS_SUCCESS) { free((void*)source); return codegen_status; };


	free((void*)source);
	return STATUS_SUCCESS;
}