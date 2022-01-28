#include "codegen.hpp"

// CodeGenerator::CodeGenerator(): _builder(__context) {}
CodeGenerator::CodeGenerator()
{
	_llvm_errstream = new llvm::raw_os_ostream(_errstream);

	// target machine stuff
	llvm::InitializeAllTargetInfos();
	llvm::InitializeAllTargets();
	llvm::InitializeAllTargetMCs();
	llvm::InitializeAllAsmParsers();
	llvm::InitializeAllAsmPrinters();

	_target_triple = llvm::sys::getDefaultTargetTriple();
	string cpu = "generic";
	string features = "";

	string error;
	const llvm::Target* target = llvm::TargetRegistry::lookupTarget(_target_triple, error);
	if (!target) { cerr << error; exit(STATUS_CODEGEN_ERROR); }

	llvm::TargetOptions opt;
	auto rm = llvm::Optional<llvm::Reloc::Model>();
	_target_machine = target->createTargetMachine(_target_triple, cpu, features, opt, rm);
}

Status CodeGenerator::generate(string infile, string outfile, const char* source, AST* astree)
{
	_outfile = outfile;
	_error_dispatcher = ErrorDispatcher(source, infile.c_str());

	// prepare
	{
		_global_init_func_block = nullptr;
		#ifdef DEBUG_NO_FOLD
		_builder = make_unique<llvm::IRBuilder<llvm::NoFolder>>(__context);
		#else
		_builder = make_unique<llvm::IRBuilder<>>(__context);
		#endif
		_top_module = make_unique<llvm::Module>("top", __context);
		_top_module->setSourceFileName(infile);
		_top_module->setDataLayout(_target_machine->createDataLayout());
		_top_module->setTargetTriple(_target_triple);

		_value_stack = new stack<llvm::Value*>();
		// _ret_val_stack = new stack<pair<llvm::Value*, llvm::Type*>>();
		_ret_type_stack = new stack<llvm::Type*>();
	}

	// walk the tree
	for(auto& node : *astree)
	{
		node->accept(this);
	}

	// finish up
	{
		if(_global_init_func_block)
		{
			_builder->SetInsertPoint(_global_init_func_block);
			_builder->CreateRetVoid();

			// if(llvm::verifyFunction(*_global_init_func_block->getParent(), _errstream))
			// {
			// 	cerr << endl;
			// 	_error_dispatcher.dispatch_error("Code Generation Error",
			// 	"LLVM verification of globals initialization function failed.");
			// 	exit(STATUS_CODEGEN_ERROR);
			// }
		}

		// verify module
		// if(llvm::verifyModule(*_top_module, _errstream))
		// {
		// 	cerr << endl;
		// 	_error_dispatcher.dispatch_error("Code Generation Error", "LLVM module verification failed.");
		// 	// exit(STATUS_CODEGEN_ERROR);
		// }

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

// =========================================

void CodeGenerator::push(llvm::Value* value)
{
	//
	_value_stack->push(value);
}

llvm::Value* CodeGenerator::pop()
{
	assert(!_value_stack->empty());
	llvm::Value* value = _value_stack->top();
	_value_stack->pop();
	return value;
}

// =========================================

llvm::AllocaInst* CodeGenerator::create_entry_block_alloca(llvm::Function *function, llvm::Value* value)
{
	llvm::IRBuilder<> TmpB(&function->getEntryBlock(), function->getEntryBlock().begin());
	return TmpB.CreateAlloca(value->getType(), 0, value->getName());
}

llvm::Value* CodeGenerator::create_cast(llvm::Value* srcval, bool srcsigned, llvm::Type* desttype, bool destsigned)
{
	llvm::Instruction::CastOps cop = llvm::CastInst::getCastOpcode(
		srcval, srcsigned, desttype, destsigned);
	return _builder->CreateCast(cop, srcval, desttype, "casttmp");
}

llvm::Type* CodeGenerator::lexical_type_to_llvm(LexicalType type)
{
	switch(type)
	{
		case TYPE_INTEGER: 	 return _builder->getInt64Ty();
		case TYPE_FLOAT:   	 return _builder->getDoubleTy();
		case TYPE_CHARACTER: return _builder->getInt8Ty();
		// case TYPE_STRING: 	 return ;
		default: assert(false);
	}
}

llvm::Type* CodeGenerator::lexical_type_to_llvm(TokenType type)
{
	switch(type)
	{
		case TOKEN_INTEGER:   return lexical_type_to_llvm(TYPE_INTEGER);
		case TOKEN_FLOAT: 	  return lexical_type_to_llvm(TYPE_FLOAT);
		case TOKEN_CHARACTER: return lexical_type_to_llvm(TYPE_CHARACTER);
		case TOKEN_STRING: 	  return lexical_type_to_llvm(TYPE_STRING);
		default: assert(false);
	}
}

// =========================================

#define VISIT(_node) void CodeGenerator::visit(_node* node)
#define ERROR_AT(token, format, ...) error_at(token, tools::fstr(format, __VA_ARGS__))
#define SCOPE_UP() map<string, pair<llvm::Value*, EviType>> oldbindings = _named_values
#define SCOPE_DOWN() _named_values = oldbindings

// === Statements ===

VISIT(FuncDeclNode)
{
	vector<llvm::Type*> params;
	for(EviType& type : node->_params) params.push_back(type.llvm_type);
	
	llvm::FunctionType* functype = llvm::FunctionType::get(node->_ret_type.llvm_type, params, false);
	llvm::Function* func = llvm::Function::Create(
		functype, llvm::Function::ExternalLinkage, node->_identifier, *_top_module);
	
	_functions[node->_identifier] = func;

	if(node->_body)
	{
		llvm::BasicBlock *block = llvm::BasicBlock::Create(__context, "entry", func);
		_builder->SetInsertPoint(block);

		// alloc each parameter
		for (int i = 0; i < params.size(); i++) {
			llvm::Argument* arg = func->getArg(i);

			// // Create an alloca for this variable.
			llvm::AllocaInst* alloca = create_entry_block_alloca(func, arg);

			// // Store the initial value into the alloca.
			_builder->CreateStore(arg, alloca);

			// Add arguments to variable symbol table.
			_named_values[arg->getName().str()].first = alloca;
			_named_values[arg->getName().str()].second = node->_params[i];
		}

		SCOPE_UP();
		_ret_type_stack->push(node->_ret_type.llvm_type);
		node->_body->accept(this);
		SCOPE_DOWN();

		// finish off function
		_ret_type_stack->pop();
		llvm::Constant* nullret = llvm::Constant::getNullValue(node->_ret_type.llvm_type);
		_builder->CreateRet(nullret);

		// verify function
		// if(llvm::verifyFunction(*func, new llvm::raw_os_ostream(cerr)))
		// {
		// 	_error_dispatcher.dispatch_error("Code Generation Error", 
		// 	tools::fstr("LLVM verification of function \"%s\" failed.", func->getName().str().c_str()).c_str());
		//			
		// 	cerr << _errstream.str() << endl;
		//	
		// 	// exit(STATUS_CODEGEN_ERROR);
		// }
	}
}

VISIT(VarDeclNode)
{
	if(node->_is_global)
	{
		_top_module->getOrInsertGlobal(node->_identifier, node->_type.llvm_type);
		llvm::GlobalVariable* global_var = _top_module->getNamedGlobal(node->_identifier);
		global_var->setLinkage(llvm::GlobalValue::CommonLinkage);
		global_var->setAlignment(llvm::MaybeAlign(node->_type.alignment));

		llvm::Constant* init = llvm::Constant::getNullValue(node->_type.llvm_type);
		global_var->setInitializer(init);
	
		// set initializer?
		if (node->_expr)
		{
			if(!_global_init_func_block)
			{
				// create global initializer function
				llvm::Function *global_init_func = llvm::getOrCreateInitFunction(*_top_module, "_global_var_init");
				_global_init_func_block = llvm::BasicBlock::Create(__context, "entry", global_init_func);
			}

			_builder->SetInsertPoint(_global_init_func_block);

			node->_expr->accept(this);

			llvm::Value* val = create_cast(pop(), true, node->_type.llvm_type, node->_type.issigned);
			_builder->CreateStore(val, global_var);
			// _builder->CreateAlignedStore(val, global_var, llvm::MaybeAlign(node->_type.alignment));
		}
		
		_named_values[node->_identifier].first = global_var;
		_named_values[node->_identifier].second = node->_type;
	}
	else // local
	{
		// get initializer?
		llvm::Value* init;
		if(node->_expr)
		{
			node->_expr->accept(this);
			init = pop();
		}
		else init = llvm::Constant::getNullValue(node->_type.llvm_type);
	
		init = create_cast(init, true, node->_type.llvm_type, node->_type.issigned);
		
		llvm::AllocaInst* alloca = _builder->CreateAlloca(node->_type.llvm_type, 0, node->_identifier);
		// _builder->CreateAlignedStore(init, alloca, llvm::MaybeAlign(node->_type.alignment));
		_builder->CreateStore(init, alloca);
	
		_named_values[node->_identifier].first = alloca;
		_named_values[node->_identifier].second = node->_type;
	}
	
}

VISIT(AssignNode)
{
	llvm::Value* var = _named_values[node->_ident].first;
	EviType type = _named_values[node->_ident].second;

	node->_expr->accept(this);
	llvm::Value* rawval = pop();

	llvm::Instruction::CastOps cop = llvm::CastInst::getCastOpcode(
		rawval, true, type.llvm_type, type.issigned);
	llvm::Value* val = _builder->CreateCast(cop, rawval, type.llvm_type);
	
	_builder->CreateStore(val, var);
}

VISIT(IfNode)
{
	node->_cond->accept(this);
	llvm::Value* cond = pop();
	
	// create comparison
	switch(node->_cond->_cast_to)
	{
		case TYPE_INTEGER:
		case TYPE_CHARACTER:
			cond = _builder->CreateICmpNE(cond, llvm::ConstantInt::getFalse(__context), "ifcond");
			break;
		case TYPE_FLOAT:
			cond = _builder->CreateFCmpONE(cond, llvm::ConstantFP::get(__context, llvm::APFloat(0.0)), "ifcond");
			break;
		default: assert(false);
	}

	llvm::Function* func = _builder->GetInsertBlock()->getParent();

	llvm::BasicBlock* thenblock = llvm::BasicBlock::Create(__context, "then", func);
	llvm::BasicBlock* elseblock = llvm::BasicBlock::Create(__context, "else", func	);
	llvm::BasicBlock* endblock = llvm::BasicBlock::Create(__context, "ifcont");
	_builder->CreateCondBr(cond, thenblock, elseblock);

	// Emit then block
	_builder->SetInsertPoint(thenblock);
	node->_then->accept(this);
	_builder->CreateBr(endblock);
	thenblock = _builder->GetInsertBlock(); // update thenblock since it changes

	// Emit else block
	_builder->SetInsertPoint(elseblock);
	if(node->_else) node->_else->accept(this);
	_builder->CreateBr(endblock);
	elseblock = _builder->GetInsertBlock(); // update elseblock since it changes

	// Emit end block.
	func->getBasicBlockList().push_back(endblock);
	_builder->SetInsertPoint(endblock);
}

VISIT(LoopNode)
{
}

VISIT(ReturnNode)
{
	// TEMPORARY
	node->_expr->accept(this);
	llvm::Type* casttype = _ret_type_stack->top();
	// _builder->CreateStore(create_cast(pop(), false, casttype, false), _ret_val_stack->top().first);
	_builder->CreateRet(create_cast(pop(), false, casttype, false));
}

VISIT(BlockNode)
{
	SCOPE_UP();
	for(StmtNode*& stmt : node->_statements) stmt->accept(this);
	SCOPE_DOWN();
}

// === Expressions ===
// all expressions should have a stack effect of 1

VISIT(LogicalNode)
{
}

VISIT(BinaryNode)
{
	node->_left->accept(this);
	node->_right->accept(this);
	
	llvm::Value* right = pop();
	llvm::Value* left = pop();

	LexicalType resulttype = node->_left->_cast_to;

	switch(node->_optype)
	{
		case TOKEN_PLUS: switch(resulttype)
			{
				case TYPE_INTEGER:   push(_builder->CreateAdd(left, right, "iaddtmp")); break;
				case TYPE_FLOAT:     push(_builder->CreateFAdd(left, right,"faddtmp")); break;
				case TYPE_CHARACTER: push(_builder->CreateAdd(left, right, "iaddtmp")); break;
				default: assert(false);
			} 
			break;
		case TOKEN_MINUS: switch(resulttype)
			{
				case TYPE_INTEGER:   push(_builder->CreateSub(left, right, "isubmp")); break;
				case TYPE_FLOAT:     push(_builder->CreateFSub(left, right,"fsubmp")); break;
				case TYPE_CHARACTER: push(_builder->CreateSub(left, right, "isubmp")); break;
				default: assert(false);
			} 
			break;
		case TOKEN_STAR: switch(resulttype)
			{
				case TYPE_INTEGER:   push(_builder->CreateMul(left, right, "imultmp")); break;
				case TYPE_FLOAT:     push(_builder->CreateFMul(left, right,"fmultmp")); break;
				case TYPE_CHARACTER: push(_builder->CreateMul(left, right, "imultmp")); break;
				default: assert(false);
			} 
			break;
		
		default: assert(false);
	}
}

VISIT(UnaryNode)
{
}

VISIT(GroupingNode)
{
	node->_expr->accept(this);
	llvm::Type* casttype = lexical_type_to_llvm(node->_cast_to);
	push(create_cast(pop(), false, casttype, false));
}


VISIT(LiteralNode)
{
	llvm::Type* type = lexical_type_to_llvm(node->_token.type);
	llvm::Constant* constant;

	switch(node->_token.type)
	{
		case TOKEN_INTEGER:
			constant = llvm::ConstantInt::get(type, node->_int_value, false);
			break;
		case TOKEN_CHARACTER:
			constant = llvm::ConstantInt::get(type, node->_char_value, false);
			break;
		case TOKEN_FLOAT:
			constant = llvm::ConstantFP::get(type, node->_float_value);
			break;
		default: assert(false);
	}

	llvm::Type* casttype = lexical_type_to_llvm(node->_cast_to);
	push(create_cast(constant, false, casttype, false));
}

VISIT(ReferenceNode)
{
	llvm::Value* var = _named_values[node->_variable].first;
	EviType type = _named_values[node->_variable].second;
	
	llvm::LoadInst* load = _builder->CreateLoad(type.llvm_type, var, "loadtmp");

	llvm::Type* casttype = lexical_type_to_llvm(node->_cast_to);
	push(create_cast(load, type.issigned, casttype, false));
}

VISIT(CallNode)
{
	llvm::Function* callee = _top_module->getFunction(node->_ident);
	assert(callee);

	vector<llvm::Value*> args;
	for(int i = 0; i < node->_arguments.size(); i++)
	{
		node->_arguments[i]->accept(this);
		llvm::Type* casttype = callee->getArg(i)->getType();
		
		args.push_back(create_cast(pop(), false, casttype, false));
	}

	push(_builder->CreateCall(callee, args, "calltmp"));
}

#undef VISIT