#include "codegen.hpp"

Status CodeGenerator::generate(string path, const char* source, AST* astree)
{
	_outfile = path;
	_error_dispatcher = ErrorDispatcher(source, path.c_str());
	_value_stack = stack<llvm::Value*>();

	// prepare
	{
		// init llvm stuff
		_top_module = make_unique<llvm::Module>(LLVM_MODULE_TOP_NAME, __context);
		// _current_module = _top_module;

		// create global initializer function
		llvm::Function *global_init_func = llvm::getOrCreateInitFunction(*_top_module, "_global_var_init");
		_global_init_func_block = llvm::BasicBlock::Create(__context, "entry", global_init_func);
	}

	// walk the tree
	for(auto& node : *astree)
	{
		node->accept(this);
	}

	// finish up
	{
		ofstream std_file_stream(_outfile);
		llvm::raw_os_ostream file_stream(std_file_stream);
		_top_module->print(file_stream, nullptr);
		file_stream.flush();
	}

	// done!
	DEBUG_PRINT_MSG("Codegen done!");
	return STATUS_SUCCESS;	
}

// =========================================

void CodeGenerator::error_at(Token *token, string message)
{
	_error_dispatcher.dispatch_error_at(token, "Code Generation Error", message.c_str());

	// print token
	cerr << endl;
	_error_dispatcher.dispatch_token_marked(token);
	cerr << endl;
	
	exit(STATUS_CODEGEN_ERROR);
}

void CodeGenerator::warning_at(Token *token, string message)
{
	// just a lil warnign
	_error_dispatcher.dispatch_warning_at(token, "Code Generation Warning", message.c_str());
}

void CodeGenerator::push(llvm::Value* value)
{
	//
	_value_stack.push(value);
}

llvm::Value* CodeGenerator::pop()
{
	assert(!_value_stack.empty());
	llvm::Value* value = _value_stack.top();
	_value_stack.pop();
	return value;
}

// =========================================

#define VISIT(_node) void CodeGenerator::visit(_node* node)
#define ERROR_AT(token, format, ...) error_at(token, tools::fstr(format, __VA_ARGS__))

// === Statements ===

VISIT(FuncDeclNode)
{
	node->_body->accept(this);
}

VISIT(VarDeclNode)
{
	if(node->_is_global)
	{
		_top_module->getOrInsertGlobal(node->_identifier, node->_type._llvm_type);
		llvm::GlobalVariable *global_var = _top_module->getNamedGlobal(node->_identifier);
		global_var->setLinkage(llvm::GlobalValue::CommonLinkage);
		global_var->setAlignment(llvm::MaybeAlign(node->_type._alignment));
	
		// set initializer?
		if (node->_expr)
		{
			_builder.SetInsertPoint(_global_init_func_block);

			node->_expr->accept(this);
			_builder.CreateStore(pop(), global_var);
			
			_builder.CreateRetVoid();
		}
	}
}

VISIT(AssignNode)
{
}

VISIT(IfNode)
{
}

VISIT(LoopNode)
{
}

VISIT(ReturnNode)
{
}

VISIT(BlockNode)
{
	for(StmtNode*& stmt : node->_statements) stmt->accept(this);
}

// === Expressions ===
// all expressions should have a stack effect of 1

VISIT(LogicalNode)
{
}

VISIT(BinaryNode)
{
}

VISIT(UnaryNode)
{
}

VISIT(GroupingNode)
{
}


VISIT(LiteralNode)
{
	switch(node->_token.type)
	{
		case TOKEN_INTEGER:
		{
			llvm::Constant* constant = llvm::ConstantInt::get(
				_builder.getInt128Ty(), llvm::APInt(node->_value.int_value, 128));
			
			push(constant);
			break;
		}
		default: assert(false);
	}
}

VISIT(ReferenceNode)
{
}

VISIT(CallNode)
{
}

#undef VISIT