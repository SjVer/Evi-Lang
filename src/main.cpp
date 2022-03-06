#define ARGP_NO_EXIT
#define ARGP_NO_HELP
#include <argp.h>
#include "common.hpp"
#include "parser.hpp"
#include "typechecker.hpp"
#include "preprocessor.hpp"
#include "codegen.hpp"
#include "debug.hpp"

// ================= arg stuff =======================

const char *argp_program_version = APP_NAME " " APP_VERSION " (MIT)";
const char *argp_program_bug_address = EMAIL;
static char args_doc[] = "file...";

#define ARGS_COUNT 1
#define MAX_LINKED 0xff

#define ARG_EMIT_LLVM 1
#define ARG_GEN_AST 2
#define ARG_LD_FLAGS 3

/* This structure is used by main to communicate with parse_opt. */
struct arguments
{
	char *args[ARGS_COUNT];	/* script */
	char *outfile;
	int linkedc = 0;
	char *linked[MAX_LINKED];
	int verbose = 0;		/* The -v flag */
	bool preprocess_only = false;
	bool emit_llvm = false;
	bool generate_ast = false;
	bool compile_only = false;
	bool output_given = false;
};

static struct argp_option options[] =
{
	{"help", 'h', 0, 0, "Display a help message."},
	{"verbose", 'v', 0, 0, "Produce verbose output."},
	{"output",  'o', "OUTFILE", 0, "Output to OUTFILE instead of to standard output."},
	{0, 'E', 0, 0, "Preprocess only but do not compile or link."},
	{"link", 'l', "FILE", 0, "Link with FILE."},
	{"include", 'i', "DIRECTORY", 0, "Add DIRECTORY to include search path."},
	{"compile-only", 'c', 0, 0, "Compile and assemble but do not link."},
	{"emit-llvm",  ARG_EMIT_LLVM, 0, 0, "Emit llvm IR instead of an executable."},
	{"generate-ast",  ARG_GEN_AST, 0, 0, "Generate AST image (for debugging purposes)."},
	{"ld-flags",  ARG_LD_FLAGS, 0, 0, "Display the flags passed to the linker."},
	{0}
};

static error_t parse_opt(int key, char *arg, struct argp_state *state);

static char *doc = strdup(tools::fstr(
	APP_DOC, APP_NAME, EMAIL, LINK, __DATE__, __TIME__, OS_NAME, COMPILER
	).c_str());
static struct argp argp = {options, parse_opt, args_doc, doc};

static error_t parse_opt(int key, char *arg, struct argp_state *state)
{
	struct arguments *arguments = (struct arguments*)state->input;

	switch (key)
	{
	case 'h':
		argp_help(&argp, stdout, ARGP_HELP_STD_HELP, APP_NAME);
		exit(0);
	case 'v':
		arguments->verbose += 1;
		break;
	case 'o':
		arguments->outfile = arg;
		arguments->output_given = true;
		break;

	case 'E':
		arguments->preprocess_only = true;
		break;
	case 'l':
	{
		if(arguments->linkedc == MAX_LINKED)
		{
			cerr << tools::fstr("[evi] Error: Cannot link more than %d extra files.", MAX_LINKED) << endl;
			// argp_usage(state);
			ABORT(STATUS_CLI_ERROR);
		}
		else
		{
			arguments->linked[arguments->linkedc] = arg;
			arguments->linkedc++;
		}
		break;
	}
	case 'i':
	{
		if(include_paths_count == MAX_INCLUDE_PATHS)
		{
			cerr << tools::fstr("[evi] Error: Cannot have more than %d include search paths.", MAX_INCLUDE_PATHS) << endl;
			// argp_usage(state);
			ABORT(STATUS_CLI_ERROR);
		}
		else
		{
			include_paths[include_paths_count] = arg;
			include_paths_count++;
		}
		break;
	}

	case 'c':
		arguments->compile_only = true;
		break;
	case ARG_EMIT_LLVM:
		arguments->emit_llvm = true;
		break;

	case ARG_GEN_AST:
		arguments->generate_ast = true;
		break;

	case ARG_LD_FLAGS:
	{
		const char* infile = "<object-file>";
		const char* outfile = "<output-file>";
		string cccommand;
		for(int i = 0; i < LD_ARGC; i++) { cccommand += (const char*[]){LD_ARGS}[i]; cccommand += " "; }
		cout << cccommand << endl;
		exit(0);
		break;
	}

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

// ================================

int main(int argc, char **argv)
{
	// ========= argument stuff =========

	struct arguments arguments;
	include_paths_count = 0;

	/* Where the magic happens */
	if(argp_parse(&argp, argc, argv, 0, 0, &arguments)) return STATUS_CLI_ERROR;

	// figure out output file name
	if(!arguments.output_given)
	{
		arguments.outfile = strdup(arguments.args[0]);
		arguments.outfile = strtok(arguments.outfile, ".");

		if(arguments.preprocess_only)
			arguments.outfile = strdup(tools::fstr("%s.evii", arguments.outfile).c_str());
		else if(arguments.compile_only && !arguments.emit_llvm)
			arguments.outfile = strdup(tools::fstr("%s.o", arguments.outfile).c_str());
		else if(arguments.emit_llvm) 
			arguments.outfile = strdup(tools::fstr("%s.ll", arguments.outfile).c_str());
	}

	// ===================================

	AST astree;
	const char* source = strdup(tools::readf(arguments.args[0]).c_str());

	init_builtin_evi_types();
	Status status;


	// preprocess
	Preprocessor* prepr = new Preprocessor();
	status = prepr->preprocess(arguments.args[0], &source);
	if(status != STATUS_SUCCESS) ABORT(status);
	if(arguments.preprocess_only) { tools::writef(arguments.outfile, source); return STATUS_SUCCESS; }


	// parse program
	Parser* parser = new Parser();
	status = parser->parse(arguments.args[0], source, &astree);
	if(status != STATUS_SUCCESS) ABORT(status);


	// type check
	TypeChecker* checker = new TypeChecker();
	status = checker->check(arguments.args[0], source, &astree);
	if(status != STATUS_SUCCESS) ABORT(status);


	// generate visualization
	if(arguments.generate_ast)
	{
		ASTVisualizer().visualize(string(arguments.args[0]) + ".svg", &astree);
		cout << "[evi] AST image written to \"" + string(arguments.args[0]) + ".svg\"." << endl;
		exit(STATUS_SUCCESS);
	}


	// codegen
	CodeGenerator* codegen = new CodeGenerator();
	status = codegen->generate(arguments.args[0], arguments.outfile, source, &astree);
	if(status != STATUS_SUCCESS) ABORT(status);


	// output
	if(arguments.compile_only && !arguments.emit_llvm) status = codegen->emit_object(arguments.outfile);
	else if(arguments.emit_llvm) status = codegen->emit_llvm(arguments.outfile);
	else status = codegen->emit_binary(arguments.outfile, (const char**)arguments.linked, arguments.linkedc);
	if(status != STATUS_SUCCESS) ABORT(status);


	free((void*)source);
	return STATUS_SUCCESS;
}