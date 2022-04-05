#define ARGP_NO_EXIT
#define ARGP_NO_HELP

#include <argp.h>
#include <regex>
#include "lint.hpp"

#include "common.hpp"
#include "parser.hpp"
#include "typechecker.hpp"
#include "preprocessor.hpp"
#include "codegen.hpp"
#include "visualizer.hpp"

// ================= arg stuff =======================

const char *argp_program_version = APP_NAME " " APP_VERSION;
const char *argp_program_bug_address = EMAIL;
static char args_doc[] = "files...";

#define MAX_INFILES 0xff
#define MAX_LINKED 0xff

#define ARG_EMIT_LLVM 1
#define ARG_GEN_AST 2
#define ARG_LD_FLAGS 3
#define ARG_LINT_TYPE 4
#define ARG_LINT_POS 5
#define ARG_LINT_TAB_WIDTH 6
#define ARG_STD_DIR 7
#define ARG_STATLIB_DIR 8

struct arguments
{
	char *infiles[MAX_INFILES];
	int infilesc = 0;
	char *outfile;
	int linkedc = 0;
	char *linked[MAX_LINKED];
	int verbose = 0;
	bool preprocess_only = false;
	bool emit_llvm = false;
	bool generate_ast = false;
	bool compile_only = false;
	bool output_given = false;
	bool debug = false;
	OptimizationType optimization = OPTIMIZE_O3;
};

static struct argp_option options[] =
{
	{"help", 				'h', 			 0, 		  0, "Display a help message."},
	{"version", 			'V', 			 0, 		  0, "Display compiler version information."},
	{"usage", 				'u', 			 0, 		  0, "Display a usage information message."},
	{"verbose", 			'v', 			 0, 		  0, "Produce verbose output."},

	{"output",  			'o', 			 "OUTFILE",   0, "Output to OUTFILE instead of to standard output."},
	{0,  					'O', 			 "LEVEL",     0, "Set the optimization level."},
	{"link", 				'l', 			 "FILE", 	  0, "Link with FILE."},
	{"include", 			'i', 			 "DIRECTORY", 0, "Add DIRECTORY to include search path."},

	{"debug", 				'd', 			 0, 		  0, "Generate source-level debug information."},
	{"preprocess-only", 	'p', 			 0, 		  0, "Preprocess only but do not compile or link."},
	{"compile-only", 		'c', 			 0, 		  0, "Compile and assemble but do not link."},
	{"emit-llvm",  			ARG_EMIT_LLVM, 	 0, 		  0, "Emit llvm IR instead of an executable."},
	{"generate-ast",  		ARG_GEN_AST, 	 0, 		  0, "Generate AST image (for debugging purposes)."},

	{"print-ld-flags",  	ARG_LD_FLAGS, 	 0, 		  0, "Display the flags passed to the linker."},
	{"print-stdlib-dir", 	ARG_STD_DIR, 	 0, 		  0, "Display the standard library header directory."},
	{"print-staticlib-dir", ARG_STATLIB_DIR, 0, 		  0, "Display the directory of the evi static library."},

	{"lint-type", 			ARG_LINT_TYPE, 		"TYPE",   OPTION_HIDDEN | OPTION_NO_USAGE, 0},
	{"lint-pos", 			ARG_LINT_POS, 		"POS", 	  OPTION_HIDDEN | OPTION_NO_USAGE, 0},
	{"lint-tab-width", 		ARG_LINT_TAB_WIDTH, "WIDTH",  OPTION_HIDDEN | OPTION_NO_USAGE, 0},

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
	case 'V':
		cout << argp_program_version << endl;
		exit(0);
	case 'u':
		argp_usage(state);
		exit(0);
	case 'v':
		arguments->verbose += 1;
		break;

	case 'o':
		arguments->outfile = arg;
		arguments->output_given = true;
		break;
	case 'O':
	{
		bool correct = false;

		if(strlen(arg) == 1) switch(arg[0])
		{
			case OPTIMIZE_O0:
			case OPTIMIZE_O1:
			case OPTIMIZE_O2:
			case OPTIMIZE_O3:
			case OPTIMIZE_On:
			case OPTIMIZE_Os:
			case OPTIMIZE_Oz:
				correct = true;
				break;
			default:
				correct = false;
				break;
		}			

		if(!correct)
		{
			cerr << "[evi] CLI Error: Invalid optimization level: " << arg << endl;
			cerr << "[evi] Note: Valid levels: '0', '1', '2', '3', 'n', 's', 'z'" << endl;
			ABORT(STATUS_CLI_ERROR);
		}

		arguments->optimization = (OptimizationType)(arg[0]);
		break;
	}
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

	case 'd':
		arguments->debug = true;
		break;
	case 'p':
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
	case ARG_STD_DIR:
	{
		cout << STDLIB_DIR << endl;
		exit(0);
		break;
	}
	case ARG_STATLIB_DIR:
	{
		cout << STATICLIB_DIR << endl;
		exit(0);
		break;
	}

	case ARG_LINT_TYPE:
	{
		LintType type = get_lint_type(arg);
		if(type == LINT_NONE)
		{
			cerr << "[evi] CLI Error: Invalid lint type: " << arg << endl;
			ABORT(STATUS_CLI_ERROR);
		}
		lint_args.type = type;
		break;
	}
	case ARG_LINT_POS:
	{
		cmatch match;
		if(!regex_match(arg, match, regex(LINT_POS_REGEX)))
		{
			cerr << "[evi] CLI Error: Invalid lint position: " << arg << endl;
			ABORT(STATUS_CLI_ERROR);
		}

		lint_args.pos[0] = stoi(match[1].str(), 0, 10);
		lint_args.pos[1] = stoi(match[2].str(), 0, 10);
		// lint_pos_given = true;

		break;
	}
	case ARG_LINT_TAB_WIDTH:
	{
		cmatch match;
		if(!regex_match(arg, match, regex("^([0-9]+)$")))
		{
			cerr << "[evi] CLI Error: Invalid lint tab width: " << arg << endl;
			ABORT(STATUS_CLI_ERROR);
		}

		lint_args.tab_width = stoi(match[1].str(), 0, 10);

		break;
	}

	case ARGP_KEY_ARG:
	{
		if(arguments->infilesc == MAX_INFILES)
		{
			cerr << tools::fstr("[evi] Error: Cannot compile more than %d evi files.", MAX_INFILES) << endl;
			ABORT(STATUS_CLI_ERROR);
		}
		else
		{
			arguments->infiles[arguments->infilesc] = arg;
			arguments->infilesc++;
		}
		break;
	}
	case ARGP_KEY_END:
	{
		if(!arguments->infilesc && !arguments->linkedc) argp_usage(state);
		break;
	}
	default: return ARGP_ERR_UNKNOWN;
	}
	return 0;
}

// ================================

void check_main_function(AST* astree)
{
	if(lint_args.type != LINT_NONE) return;

	// search for main func declaration
	FuncDeclNode* mainfunc = nullptr;
	for(auto& node : *astree)
		if(!mainfunc && dynamic_cast<FuncDeclNode*>(node) && ((FuncDeclNode*)node)->_identifier == "main")
				mainfunc = (FuncDeclNode*)node;

	ParsedType* rettype = PTYPE(TYPE_INTEGER);
	ParsedType* argone = PTYPE(TYPE_INTEGER);
	ParsedType* argtwo = PTYPE(TYPE_CHARACTER)->copy_pointer_to()->copy_pointer_to();
	argtwo->_is_constant = true;

	if(!mainfunc) // main func not found
		ErrorDispatcher().warning("Warning", "Function " COLOR_BOLD "'main'" COLOR_NONE " not declared.");

	else if(!mainfunc->_ret_type->eq(rettype, true)) // wrong return type
		ErrorDispatcher().warning_at_token(&mainfunc->_token, "Warning",
			"Return type of function " COLOR_BOLD "'main'" COLOR_NONE " is not an integer type.");

	else if(mainfunc->_params.size() != 0 && (mainfunc->_params.size() != 2 ||
			!mainfunc->_params[0]->eq(argone, true) || !mainfunc->_params[1]->eq(argtwo, true))) // incorrect args
		ErrorDispatcher().warning_at_token(&mainfunc->_token, "Warning", 
			"Function " COLOR_BOLD "'main'" COLOR_NONE " does not have parameters similar to " COLOR_BOLD "'i32 !chr**'" COLOR_NONE ".");
}

int main(int argc, char **argv)
{
	// ========= argument stuff =========

	struct arguments arguments;
	include_paths_count = 0;

	/* Where the magic happens */
	if(argp_parse(&argp, argc, argv, 0, 0, &arguments)) ABORT(STATUS_CLI_ERROR);

	// figure out output file name
	if(!arguments.output_given)
	{
		arguments.outfile = strdup(arguments.infilesc ? arguments.infiles[0] : arguments.linked[0]);
		arguments.outfile = strtok(arguments.outfile, ".");

		if(arguments.preprocess_only)
			arguments.outfile = strdup(tools::fstr("%s.evii", arguments.outfile).c_str());
		else if(arguments.compile_only && !arguments.emit_llvm)
			arguments.outfile = strdup(tools::fstr("%s.o", arguments.outfile).c_str());
		else if(arguments.emit_llvm) 
			arguments.outfile = strdup(tools::fstr("%s.ll", arguments.outfile).c_str());
	}

	// ===================================

	// temp
	if(!arguments.infilesc)
	{
		cerr << "[evi] CLI Error: Expected at least one Evi file." << endl;
		ABORT(STATUS_CLI_ERROR);
	}

	// ===================================

	AST astree;
	const char* source = strdup(tools::readf(arguments.infiles[0]).c_str());
	init_builtin_evi_types();
	Status status;

	#define ABORT_IF_UNSUCCESSFULL() if(status != STATUS_SUCCESS) { if(lint_args.type == LINT_GET_DIAGNOSTICS) \
									 { LINT_OUTPUT_END_PLAIN_ARRAY(); cout << lint_output; exit(0); } ABORT(status); }
	if(lint_args.type == LINT_GET_DIAGNOSTICS) LINT_OUTPUT_START_PLAIN_ARRAY();


	// preprocess
	Preprocessor* prepr = new Preprocessor();
	status = prepr->preprocess(arguments.infiles[0], &source);
	ABORT_IF_UNSUCCESSFULL();
	if(arguments.preprocess_only) { tools::writef(arguments.outfile, source); return STATUS_SUCCESS; }


	// parse program
	Parser* parser = new Parser();
	status = parser->parse(arguments.infiles[0], source, &astree);
	ABORT_IF_UNSUCCESSFULL();


	// check for function @main i32 (...)
	if(!arguments.compile_only && !arguments.emit_llvm && !arguments.generate_ast)
		check_main_function(&astree);


	// type check
	TypeChecker* checker = new TypeChecker();
	status = checker->check(arguments.infiles[0], source, &astree);
	ABORT_IF_UNSUCCESSFULL();


	// finish linting
	if(lint_args.type == LINT_GET_DIAGNOSTICS)
	{
		LINT_OUTPUT_END_PLAIN_ARRAY();
		cout << lint_output;
		exit(0);
	}
	else if(lint_args.type != LINT_NONE)
	{
		if(lint_output.length() < 3) lint_output = "{}";
		cout << lint_output;
		exit(0);
	}


	// generate visualization
	if(arguments.generate_ast)
	{
		ASTVisualizer().visualize(string(arguments.infiles[0]) + ".svg", &astree);
		cout << "[evi] AST image written to \"" + string(arguments.infiles[0]) + ".svg\"." << endl;
		exit(STATUS_SUCCESS);
	}


	// codegen
	CodeGenerator* codegen = new CodeGenerator();
	status = codegen->generate(arguments.infiles[0], arguments.outfile, source, &astree, arguments.optimization);
	ABORT_IF_UNSUCCESSFULL();


	// output
	if(arguments.compile_only && !arguments.emit_llvm) status = codegen->emit_object(arguments.outfile);
	else if(arguments.emit_llvm) status = codegen->emit_llvm(arguments.outfile);
	else status = codegen->emit_binary(arguments.outfile, (const char**)arguments.linked, arguments.linkedc);
	ABORT_IF_UNSUCCESSFULL();


	free((void*)source);
	DEBUG_PRINT_MSG("Exited sucessfully.");
	return STATUS_SUCCESS;
}