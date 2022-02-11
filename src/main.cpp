#define ARGP_NO_EXIT
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
#define ARG_EMIT_LLVM 1
#define ARG_GEN_AST 2
#define ARG_C_FLAGS 3

/* This structure is used by main to communicate with parse_opt. */
struct arguments
{
	char *args[ARGS_COUNT];	/* script */
	char *outfile;
	int verbose = 0;		/* The -v flag */
	bool preprocess_only = false;
	bool emit_llvm = false;
	bool generate_ast = false;
	bool compile_only = false;
	bool output_given = false;
};

static struct argp_option options[] =
{
	{"verbose", 'v', 0, 0, "Produce verbose output."},
	{"output",  'o', "OUTFILE", 0, "Output to OUTFILE instead of to standard output."},
	{0, 'E', 0, 0, "Preprocess only but do not compile or link."},
	{"compile-only", 'c', 0, 0, "Compile and assemble but do not link."},
	{"emit-llvm",  ARG_EMIT_LLVM, 0, 0, "Emit llvm IR instead of an executable."},
	{"generate-ast",  ARG_GEN_AST, 0, 0, "Generate AST image (for debugging purposes)."},
	{"c-flags",  ARG_C_FLAGS, 0, 0, "Display the flags passed to the c compiler for linkage."},
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
	case 'E':
		arguments->preprocess_only = true;
		break;
	case 'c':
		arguments->compile_only = true;
		break;
	case ARG_EMIT_LLVM:
		arguments->emit_llvm = true;
		break;
	case ARG_GEN_AST:
		arguments->generate_ast = true;
		break;
	case ARG_C_FLAGS:
	{
		const char* infile = "<object-file>";
		const char* outfile = "<output-file>";
		string cccommand;
		for(int i = 0; i < CC_ARGC; i++) { cccommand += (const char*[]){CC_ARGS}[i]; cccommand += " "; }
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

		if(arguments.compile_only && !arguments.emit_llvm)
			arguments.outfile = strdup(tools::fstr("%s.o", arguments.outfile).c_str());
		else if(arguments.emit_llvm) 
			arguments.outfile = strdup(tools::fstr("%s.ll", arguments.outfile).c_str());
	}

	// ===================================

	AST astree;
	const char* source = strdup(tools::readf(arguments.args[0]).c_str());

	init_builtin_evi_types();
	Status status;

	// parse program
	Parser parser;
	status = parser.parse(arguments.args[0], source, &astree);
	if(status != STATUS_SUCCESS) { free((void*)source); return status; }

	
	// generate visualization
	if(arguments.generate_ast)
	{
		ASTVisualizer().visualize(string(arguments.args[0]) + ".svg", &astree);
		cout << "[evi] AST image written to \"" + string(arguments.args[0]) + ".svg\"." << endl;
	}

	// TODO: make sure that e.g. --emit-llvm and --compile-only don't go together :)
	//		 then start working on the curseth preprocessoreth.

	// type check
	TypeChecker checker;
	status = checker.check(arguments.args[0], source, &astree);
	if(status != STATUS_SUCCESS) { free((void*)source); return status; }


	// codegen
	CodeGenerator* codegen = new CodeGenerator();
	status = codegen->generate(arguments.args[0], arguments.outfile, source, &astree);
	if(status != STATUS_SUCCESS) { free((void*)source); return status; };


	// output
	if(arguments.compile_only && !arguments.emit_llvm) status = codegen->emit_object(arguments.outfile);
	else if(arguments.emit_llvm) status = codegen->emit_llvm(arguments.outfile);
	else status = codegen->emit_binary(arguments.outfile);
	if(status != STATUS_SUCCESS) { free((void*)source); return status; };


	free((void*)source);
	return STATUS_SUCCESS;
}