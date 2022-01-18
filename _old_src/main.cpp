#include <argp.h>
#include "common.hpp"
#include "vm.hpp"

// ================= arg stuff =======================

const char *argp_program_version = APP_NAME " " APP_VERSION;
const char *argp_program_bug_address = EMAIL;

#define ARGS_COUNT 1

/* This structure is used by main to communicate with parse_opt. */
struct arguments
{
	char *args[ARGS_COUNT];	/* script */
	int verbose;			/* The -v flag */
	
	bool _repl;
};

static struct argp_option options[] = {
		// longname, shortname, arg, idk, help
		{"verbose", 'v', 0, 0, "Enable verbose output"},
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
		if (state->arg_num < 1)
		{
			// insert fake filename and do repl
			arguments->args[state->arg_num] = "NONE";
			state->arg_num++;
			arguments->_repl = true;
		}
		if (state->arg_num < ARGS_COUNT)
		{
			argp_usage(state);
		}
		break;
	}
	default: return ARGP_ERR_UNKNOWN;
	}
	return 0;
}

static char args_doc[] = "[script]";
static char *doc = fstr(APP_DOC, APP_NAME, EMAIL, LINK, __DATE__, __TIME__, OS, COMPILER);
static struct argp argp = {options, parse_opt, args_doc, doc};

// ================================

// run the REPL
static void repl()
{
	printf("%s", fstr(WELCOME_MESSAGE, 
		EVI_VERSION_0, EVI_VERSION_1, EVI_VERSION_2,
		__DATE__, __TIME__, COMPILER, __VERSION__, OS));

	// defineNativeFn("Help", helpNative, 0);
	VM vm("<stdin>", "", true);

	string buffer;
	char line[1024];
	int depth = 0;

	for (;;)
	{
		#ifdef DEBUG
			printf("\n");
		#endif

		// prompt
		printf(depth == 0 ? PROMPT_NORM : PROMPT_IND);
	
		// get input
		if (!fgets(line, sizeof(line), stdin))
		{
			printf("\n");
			break;
		}

		// trim trailing whitespaces
		{
			int index, i;

			/* Set default index */
			index = -1;

			/* Find last index of non-white space character */
			i = 0;
			while (line[i] != '\0')
			{
				if (line[i] != ' ' && line[i] != '\t' && line[i] != '\n')
				{
					index = i;
				}

				i++;
			}

			/* Mark next character to last non-white space character as NULL */
			line[index + 1] = '\0';
		}

		// handle blocks
		#define eq(ch) (line[i] == ch)
		for (int i = 0; i < strlen(line); i++)
		{
			
			if (eq('{') || eq('['))
				depth++;
			else if (eq('}') || eq(']'))
				depth = depth-- >= 0 ? depth : 0;
		}
		#undef eq

		buffer.append(string(buffer == "" ? "" : "\n") + string(line));

		// interpret
		if (depth == 0)
		{
			vm.source = buffer;
			vm.interpret();
			buffer = "";
		}
	}

	exit(0);
}

// run the given file
static void runFile(const char *path)
{
	// char *source = readFile(path);
	// InterpretResult result = INTERPRET_SUCCESS;

	VM vm(path, readFile(path), false);
	InterpretResult result = vm.interpret();

	if (result.status != INTERPRET_SUCCESS) exit(result.status);
	else exit(result.exitCode);
}

int main(int argc, char **argv)
{
	// ========= argument stuff =========

	struct arguments arguments;

	/* Set argument defaults */
	arguments.verbose = false;

	/* Where the magic happens */
	argp_parse(&argp, argc, argv, 0, 0, &arguments);

	// printf("script: '%s', in_repl: %s, verbose: %d\n",
	// 	arguments.args[0], arguments._repl ? "true" : "false", arguments.verbose);

	// ===================================

	if (arguments._repl)
		repl();
	else
		runFile(arguments.args[0]);

	return 0;
}