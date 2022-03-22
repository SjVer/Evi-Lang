#include "codegen.hpp"
#include <unistd.h>

CodeGenerator::CodeGenerator()
{
	_errstream = new llvm::raw_os_ostream(cerr);

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
	if (!target) { cerr << error; ABORT(STATUS_CODEGEN_ERROR); }

	llvm::TargetOptions opt;
	auto rm = llvm::Optional<llvm::Reloc::Model>();
	_target_machine = target->createTargetMachine(_target_triple, cpu, features, opt, rm);
}

Status CodeGenerator::emit_llvm(const char* outfile)
{
	ofstream std_file_stream(outfile);
	llvm::raw_os_ostream file_stream(std_file_stream);
	_top_module->print(file_stream, nullptr);
	file_stream.flush();
	return STATUS_SUCCESS;
}

Status CodeGenerator::emit_object(const char* outfile)
{
	error_code errcode;
	llvm::raw_fd_ostream dest(outfile, errcode, llvm::sys::fs::OF_None);

	if (errcode)
	{
		_error_dispatcher.error("Code Generation Error", 
			tools::fstr("Could not open file \"%s\": %s.", outfile, errcode.message().c_str()).c_str());
		ABORT(STATUS_CODEGEN_ERROR);
	}

	llvm::legacy::PassManager pass;
	auto filetype = llvm::CGFT_ObjectFile;

	if (_target_machine->addPassesToEmitFile(pass, dest, nullptr, filetype))
	{
		_error_dispatcher.error("Code Generation Error",
			"Target machine incompatible with object file type.");
		remove(outfile);
		ABORT(STATUS_CODEGEN_ERROR);
	}

	pass.run(*_top_module);
	dest.flush();
	return STATUS_SUCCESS;
}

Status CodeGenerator::emit_binary(const char* outfile, const char** linked, int linkedc)
{
	// write to temporary object file
	const char* objfile = strdup(tools::fstr(TEMP_OBJ_FILE_TEMPLATE, _infile, time(0)).c_str());
	DEBUG_PRINT_F_MSG("Emitting object file... (%s)", objfile);
	Status objstatus = emit_object(objfile);
	if(objstatus != STATUS_SUCCESS) return objstatus; 

	// object file written, now invoke llc
	// int ldstatus = execl(LD_PATH, CC_ARGS, NULL);
	string ldcommand; const char* infile = objfile;
	for(int i = 0; i < LD_ARGC; i++) { ldcommand += (const char*[]){LD_ARGS}[i]; ldcommand += " "; }
	for(int i = 0; i < linkedc; i++) ldcommand += string(linked[i]) + " ";

	DEBUG_PRINT_MSG("Invoking linker (" LD_PATH " with stdlib at " STATICLIB_DIR ")");
	DEBUG_PRINT_F_MSG("Linker command: %s", ldcommand.c_str());

	int ldstatus = system(ldcommand.c_str());
	if(ldstatus)
	{
		_error_dispatcher.error("Linking Error", tools::fstr(
			"Linking with " LD_PATH " failed with code %d.", ldstatus).c_str());
		remove(objfile);
		return STATUS_OUTPUT_ERROR;
	}

	// clean up
	DEBUG_PRINT_F_MSG("Cleaning up object file... (%s)", objfile);
	remove(objfile);
	free((void*)objfile);
	return STATUS_SUCCESS;
}

Status CodeGenerator::generate(const char* infile, const char* outfile, const char* source, AST* astree)
{
	_outfile = strdup(outfile);
	_infile = strdup(infile);
	_error_dispatcher = ErrorDispatcher();

	prepare();

	// walk the tree
	for(auto& node : *astree)
	{
		node->accept(this);
	}

	finish();

	// done!
	DEBUG_PRINT_MSG("Codegen done!");
	return STATUS_SUCCESS;	
}

// =========================================

void CodeGenerator::prepare()
{
	_global_init_func_block = nullptr;
	#ifdef DEBUG_NO_FOLD
	_builder = make_unique<llvm::IRBuilder<llvm::NoFolder>>(__context);
	#else
	_builder = make_unique<llvm::IRBuilder<>>(__context);
	#endif
	_top_module = make_unique<llvm::Module>("top", __context);
	_top_module->setSourceFileName(_infile);
	_top_module->setDataLayout(_target_machine->createDataLayout());
	_top_module->setTargetTriple(_target_triple);

	_value_stack = new stack<llvm::Value*>();
}

void CodeGenerator::finish()
{
	bool invalid = false;

	if(_global_init_func_block)
	{
		_builder->SetInsertPoint(_global_init_func_block);
		_builder->CreateRetVoid();

		if(llvm::verifyFunction(*_global_init_func_block->getParent(), nullptr))
		{
			_error_dispatcher.error("Code Generation Error", 
				"LLVM verification of globals initialization function failed: ");
			llvm::verifyFunction(*_global_init_func_block->getParent(), _errstream);
			cerr << endl << endl;
			invalid = true;
		}
	}
	
	// verify module
	if(llvm::verifyModule(*_top_module, nullptr))
	{
		_error_dispatcher.error("Code Generation Error", "LLVM module verification failed: ");
		llvm::verifyModule(*_top_module, _errstream);
		cerr << endl << endl;
		invalid = true;
	}

	#ifdef DEBUG
	DEBUG_PRINT_MSG("Generated LLVM IR:");
	llvm::raw_os_ostream file_stream(cout);
	_top_module->print(file_stream, nullptr);
	file_stream.flush();
	#endif

	// if(invalid) ABORT(STATUS_CODEGEN_ERROR);
}

// =========================================

void CodeGenerator::error_at(Token *token, string message)
{
	_error_dispatcher.error_at_token(token, "Code Generation Error", message.c_str());

	// print token
	cerr << endl;
	_error_dispatcher.print_token_marked(token, COLOR_RED);
	cerr << endl;
	
	ABORT(STATUS_CODEGEN_ERROR);
}

void CodeGenerator::warning_at(Token *token, string message)
{
	// just a lil warnign
	_error_dispatcher.warning_at_token(token, "Code Generation Warning", message.c_str());
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

llvm::AllocaInst* CodeGenerator::create_entry_block_alloca(llvm::Type* ty, string name)
{
	#ifdef DEBUG_NO_FOLD
	llvm::IRBuilder<llvm::NoFolder> TmpB(&_builder->GetInsertBlock()->getParent()->getEntryBlock(), 
		_builder->GetInsertBlock()->getParent()->getEntryBlock().begin());
	#else
	llvm::IRBuilder<> TmpB(&_builder->GetInsertBlock()->getParent()->getEntryBlock(), 
		_builder->GetInsertBlock()->getParent()->getEntryBlock().begin());
	#endif
	return TmpB.CreateAlloca(ty, 0, name);
}

llvm::Value* CodeGenerator::to_bool(llvm::Value* value)
{
	llvm::Type* type = value->getType();

	if(type->isIntegerTy())
		// return _builder->CreateTrunc(value, _builder->getInt1Ty(), "itobtmp");
		return _builder->CreateICmpNE(value, llvm::ConstantInt::get(value->getType(), 0), "itobtmp");
	else if(type->isFloatTy() || type->isDoubleTy())	
		return _builder->CreateFCmpONE(value, llvm::ConstantFP::get(__context, llvm::APFloat(0.0)), "ftobtmp");
	else if(type->isPointerTy())
		return _builder->CreateICmpNE(value, llvm::Constant::getNullValue(type), "ptobtmp");

	assert(false);
}

llvm::Value* CodeGenerator::create_cast(llvm::Value* srcval, bool srcsigned, llvm::Type* desttype, bool destsigned)
{
	llvm::Instruction::CastOps cop = llvm::CastInst::getCastOpcode(
		srcval, srcsigned, desttype, destsigned);
	return _builder->CreateCast(cop, srcval, desttype, "casttmp");
}

ParsedType* CodeGenerator::from_token_type(TokenType type)
{
	switch(type)
	{
		case TOKEN_INTEGER:   return PTYPE(TYPE_INTEGER);
		case TOKEN_FLOAT: 	  return PTYPE(TYPE_FLOAT);
		case TOKEN_CHARACTER: return PTYPE(TYPE_CHARACTER);
		case TOKEN_STRING: 	  return PTYPE(TYPE_CHARACTER)->copy_pointer_to();
		default: assert(false);
	}
}

// =========================================

#define VISIT(_node) void CodeGenerator::visit(_node* node)
#define ERROR_AT(token, format, ...) error_at(token, tools::fstr(format, __VA_ARGS__))
#define SCOPE_UP() map<string, pair<llvm::Value*, ParsedType*>> oldbindings = _named_values
#define SCOPE_DOWN() _named_values = oldbindings

// === Statements ===

VISIT(FuncDeclNode)
{
	llvm::Function* func;
	vector<llvm::Type*> params;

	if(_functions.find(node->_identifier) == _functions.end())
	{
		for(ParsedType*& type : node->_params) params.push_back(type->get_llvm_type());
		
		llvm::FunctionType* functype = llvm::FunctionType::get(node->_ret_type->get_llvm_type(), params, false);
		func = llvm::Function::Create(functype, llvm::Function::ExternalLinkage, node->_identifier, *_top_module);
		
		_functions[node->_identifier] = func;
	}
	else func = _functions[node->_identifier];

	if(node->_body)
	{
		llvm::BasicBlock *block = llvm::BasicBlock::Create(__context, "entry", func);
		_builder->SetInsertPoint(block);

		SCOPE_UP();

		// alloc each parameter
		for (int i = 0; i < params.size(); i++) {
			llvm::Argument* arg = func->getArg(i);

			// // Create an alloca for this variable.
			llvm::AllocaInst* alloca = create_entry_block_alloca(arg->getType(), tools::fstr("_%d", i));

			// // Store the initial value into the alloca.
			_builder->CreateStore(arg, alloca);

			// Add arguments to variable symbol table.
			_named_values[tools::fstr("%d", i)].first = alloca;
			_named_values[tools::fstr("%d", i)].second = node->_params[i];
		}

		// eval the body
		node->_body->accept(this);
		SCOPE_DOWN();

		// finish off function
		if(func->getBasicBlockList().back().getInstList().back().isTerminator()) {}
		else if(node->_ret_type->_lexical_type != TYPE_VOID)
		{
			llvm::Constant* nullret = llvm::Constant::getNullValue(node->_ret_type->get_llvm_type());
			_builder->CreateRet(nullret);
		}
		else _builder->CreateRetVoid();

		// verify function
		// if(llvm::verifyFunction(*func, new llvm::raw_os_ostream(cerr)))
		// {
		// 	_error_dispatcher.dispatch_error("Code Generation Error", 
		// 	tools::fstr("LLVM verification of function \"%s\" failed.", func->getName().str().c_str()).c_str());
		//			
		// 	cerr << _errstream.str() << endl;
		//	
		// 	// ABORT(STATUS_CODEGEN_ERROR);
		// }
	}
}

VISIT(VarDeclNode)
{
	if(node->_is_global)
	{
		// DEBUG_PRINT_F_MSG("decl var '%s' %d", node->_identifier.c_str(), 
		// 									  node->_type->_pointer_depth);

		_top_module->getOrInsertGlobal(node->_identifier, node->_type->get_llvm_type());
		llvm::GlobalVariable* global_var = _top_module->getNamedGlobal(node->_identifier);
		global_var->setLinkage(llvm::GlobalValue::CommonLinkage);
		global_var->setAlignment(llvm::MaybeAlign(node->_type->get_alignment()));

		llvm::Constant* init = llvm::Constant::getNullValue(node->_type->get_llvm_type());
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

			llvm::Value* val = create_cast(pop(), false, node->_type->get_llvm_type(), node->_type->is_signed());
			_builder->CreateStore(val, global_var);
			// _builder->CreateAlignedStore(val, global_var, llvm::MaybeAlign(node->_type->_alignment));
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
		else init = llvm::Constant::getNullValue(node->_type->get_llvm_type());
	
		init = create_cast(init, true, node->_type->get_llvm_type(), node->_type->is_signed());
		
		llvm::AllocaInst* alloca = create_entry_block_alloca(node->_type->get_llvm_type(), node->_identifier);
		_builder->CreateStore(init, alloca);
	
		_named_values[node->_identifier].first = alloca;
		_named_values[node->_identifier].second = node->_type;
	}
}

VISIT(AssignNode)
{
	llvm::Value* target = _named_values[node->_ident].first;
	ParsedType* type = _named_values[node->_ident].second;

	// handle subscripts
	for(auto& subnode : node->_subscripts)
	{
		subnode->accept(this);
		llvm::Value* index = pop();

		target = _builder->CreateInBoundsGEP(_builder->CreateLoad(target, "asslodtmp"), index, "assgeptmp");
		type = type->copy_element_of();
	}

	node->_expr->accept(this);
	llvm::Value* rawval = pop();

	llvm::Instruction::CastOps cop = llvm::CastInst::getCastOpcode(
		rawval, true, type->get_llvm_type(), type->is_signed());
	llvm::Value* val = _builder->CreateCast(cop, rawval, type->get_llvm_type());
	
	_builder->CreateStore(val, target);
}

VISIT(IfNode)
{
	node->_cond->accept(this);
	
	// create comparison
	llvm::Value* cond = to_bool(pop());

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
	llvm::Function* func = _builder->GetInsertBlock()->getParent();
	llvm::BasicBlock* condblock = llvm::BasicBlock::Create(__context, "loopcond", func);
	llvm::BasicBlock* loopblock = llvm::BasicBlock::Create(__context, "loopbody", func);
	llvm::BasicBlock* endblock = llvm::BasicBlock::Create(__context, "loopcont", func);

	SCOPE_UP();

	// first do initializer
	if(node->_init) node->_init->accept(this);
	_builder->CreateBr(condblock);

	// then do condition
	_builder->SetInsertPoint(condblock);
	node->_cond->accept(this);
	_builder->CreateCondBr(to_bool(pop()), loopblock, endblock);
	condblock = _builder->GetInsertBlock(); // update condblock

	// finally do loop body and incrementor
	_builder->SetInsertPoint(loopblock);
	node->_body->accept(this);
	if(node->_incr) node->_incr->accept(this);
	_builder->CreateBr(condblock);
	loopblock = _builder->GetInsertBlock(); // update loopblock

	// patch up
	_builder->SetInsertPoint(endblock);
	endblock->moveAfter(loopblock);
	SCOPE_DOWN();
}

VISIT(ReturnNode)
{
	if(node->_expr)
	{
		node->_expr->accept(this);
		_builder->CreateRet(create_cast(pop(), false, node->_expected_type->get_llvm_type(), false));
	}
	else _builder->CreateRetVoid();
}

VISIT(BlockNode)
{
	if(!node->_secret)
	{
		SCOPE_UP();
		for(StmtNode*& stmt : node->_statements) stmt->accept(this);
		SCOPE_DOWN();
	}
	else for(StmtNode*& stmt : node->_statements) stmt->accept(this);
}

// === Expressions ===
// all expressions should have a stack effect of 1

VISIT(LogicalNode)
{
	llvm::Function* func = _builder->GetInsertBlock()->getParent();

	if(node->_token.type == TOKEN_QUESTION) // ternary
	{
		llvm::BasicBlock* ifblock = llvm::BasicBlock::Create(__context, "ternif", func);
		llvm::BasicBlock* elseblock = llvm::BasicBlock::Create(__context, "ternelse", func);
		llvm::BasicBlock* endblock = llvm::BasicBlock::Create(__context, "terncont", func);

		// first eval condition
		node->_left->accept(this);
		llvm::Value* cond = to_bool(pop());
		_builder->CreateCondBr(cond, ifblock, elseblock);

		// if block
		_builder->SetInsertPoint(ifblock);
		node->_middle->accept(this);
		ParsedType* casttype = node->_middle->_cast_to;
		llvm::Value* ifval = create_cast(pop(), false, casttype->get_llvm_type(), casttype->is_signed());
		_builder->CreateBr(endblock);
		ifblock = _builder->GetInsertBlock(); // update ifblock

		// else block
		endblock->moveAfter(ifblock);
		_builder->SetInsertPoint(elseblock);
		node->_right->accept(this);
		casttype = node->_right->_cast_to;
		llvm::Value* elseval = create_cast(pop(), false, casttype->get_llvm_type(), casttype->is_signed());
		_builder->CreateBr(endblock);
		elseblock = _builder->GetInsertBlock(); // update elseblock

		// patch up
		_builder->SetInsertPoint(endblock);
		endblock->moveAfter(elseblock);

		// decide on result
		llvm::PHINode* phi = _builder->CreatePHI(node->_middle->_cast_to->get_llvm_type(), 2, "ternres");
		phi->addIncoming(ifval, ifblock);
		phi->addIncoming(elseval, elseblock);

		// push(create_cast(phi, false, lexical_type_to_llvm(node->_cast_to), false));
		push(phi);
	}
	else if(node->_token.type == TOKEN_PIPE_PIPE) // or
	{
		llvm::Function* func = _builder->GetInsertBlock()->getParent();
		llvm::BasicBlock* start = _builder->GetInsertBlock();
		llvm::BasicBlock* ortwo = llvm::BasicBlock::Create(__context, "ortwo", func);
		llvm::BasicBlock* orcont = llvm::BasicBlock::Create(__context, "orcont", func);

		// first condition
		node->_left->accept(this);
		// llvm::Value* resulta = to_bool(pop());
		_builder->CreateCondBr(to_bool(pop()), orcont, ortwo);

		// second condition (only evaluated if first is false)
		_builder->SetInsertPoint(ortwo);
		node->_right->accept(this);
		llvm::Value* result = to_bool(pop());
		_builder->CreateBr(orcont);
		ortwo = _builder->GetInsertBlock(); // update ortwo block

		// patch up
		_builder->SetInsertPoint(orcont);
		orcont->moveAfter(ortwo);
		
		// decide on result
		llvm::PHINode *phi = _builder->CreatePHI(_builder->getInt1Ty(), 2, "orres");
		phi->addIncoming(_builder->getTrue(), start);
		phi->addIncoming(result, ortwo);

		// push(create_cast(phi, false, lexical_type_to_llvm(node->_cast_to), false));
		push(phi);
	}
	else if(node->_token.type == TOKEN_CARET_CARET) // xor
	{
		// first condition
		node->_left->accept(this);
		llvm::Value* lhs = to_bool(pop());

		// second condition (only evaluated if first is true)
		node->_right->accept(this);
		llvm::Value* rhs = to_bool(pop());

		// decide on result
		push(_builder->CreateXor(lhs, rhs));
	}
	else if(node->_token.type == TOKEN_AND_AND) // and
	{
		llvm::BasicBlock* start = _builder->GetInsertBlock();
		llvm::BasicBlock* andtwo = llvm::BasicBlock::Create(__context, "andtwo", func);
		llvm::BasicBlock* andcont = llvm::BasicBlock::Create(__context, "andcont", func);

		// first condition
		node->_left->accept(this);
		// llvm::Value* resulta = to_bool(pop());
		_builder->CreateCondBr(to_bool(pop()), andtwo, andcont);

		// second condition (only evaluated if first is true)
		_builder->SetInsertPoint(andtwo);
		node->_right->accept(this);
		llvm::Value* result = to_bool(pop());
		_builder->CreateBr(andcont);
		andtwo = _builder->GetInsertBlock(); // update andtwo block

		// patch up
		_builder->SetInsertPoint(andcont);
		andcont->moveAfter(andtwo);
		
		// decide on result
		llvm::PHINode *phi = _builder->CreatePHI(_builder->getInt1Ty(), 2, "andres");
		phi->addIncoming(_builder->getFalse(), start);
		phi->addIncoming(result, andtwo);

		// push(create_cast(phi, false, lexical_type_to_llvm(node->_cast_to), false));
		push(phi);
	}
	
	else assert(false);
}

VISIT(BinaryNode)
{
	node->_left->accept(this);
	node->_right->accept(this);

	ParsedType* resulttype = node->_left->_cast_to;
	
	llvm::Value* right = create_cast(pop(), resulttype->is_signed(), resulttype->get_llvm_type(), resulttype->is_signed());
	llvm::Value* left = create_cast(pop(), resulttype->is_signed(), resulttype->get_llvm_type(), resulttype->is_signed());

	switch(node->_optype)
	{
		case TOKEN_PIPE: switch(AS_LEX(resulttype))
			{
				case TYPE_INTEGER:   push(_builder->CreateOr(left, right, "ibotmp")); break;
				case TYPE_FLOAT:     push(_builder->CreateOr(left, right,"fbotmp")); break;
				case TYPE_CHARACTER: push(_builder->CreateOr(left, right, "cbotmp")); break;
				default: assert(false);
			}
			break;
		case TOKEN_CARET: switch(AS_LEX(resulttype))
			{
				case TYPE_INTEGER:   push(_builder->CreateXor(left, right, "ibxtmp")); break;
				case TYPE_FLOAT:     push(_builder->CreateXor(left, right,"fbxtmp")); break;
				case TYPE_CHARACTER: push(_builder->CreateXor(left, right, "cbxtmp")); break;
				default: assert(false);
			}
			break;
		case TOKEN_AND: switch(AS_LEX(resulttype))
			{
				case TYPE_INTEGER:   push(_builder->CreateAnd(left, right, "ibatmp")); break;
				case TYPE_FLOAT:     push(_builder->CreateAnd(left, right,"fbatmp")); break;
				case TYPE_CHARACTER: push(_builder->CreateAnd(left, right, "cbatmp")); break;
				default: assert(false);
			}
			break;

		case TOKEN_EQUAL_EQUAL: switch(AS_LEX(resulttype))
			{
				case TYPE_INTEGER:   push(_builder->CreateICmpEQ(left, right, "ieqtmp")); break;
				case TYPE_FLOAT:     push(_builder->CreateFCmpOEQ(left, right,"feqtmp")); break;
				case TYPE_CHARACTER: push(_builder->CreateICmpEQ(left, right, "ceqtmp")); break;
				default: assert(false);
			}
			break;
		case TOKEN_SLASH_EQUAL: switch(AS_LEX(resulttype))
			{
				case TYPE_INTEGER:   push(_builder->CreateICmpNE(left, right, "inetmp")); break;
				case TYPE_FLOAT:     push(_builder->CreateFCmpONE(left, right,"fnetmp")); break;
				case TYPE_CHARACTER: push(_builder->CreateICmpNE(left, right, "cnetmp")); break;
				default: assert(false);
			}
			break;

		case TOKEN_GREATER_EQUAL: switch(AS_LEX(resulttype))
			{
				case TYPE_INTEGER:   push(_builder->CreateICmpSGE(left, right, "igetmp")); break;
				case TYPE_FLOAT:     push(_builder->CreateFCmpOGE(left, right,"fgetmp")); break;
				case TYPE_CHARACTER: push(_builder->CreateICmpUGE(left, right, "cgetmp")); break;
				default: assert(false);
			}
			break;
		case TOKEN_LESS_EQUAL: switch(AS_LEX(resulttype))
			{
				case TYPE_INTEGER:   push(_builder->CreateICmpSLE(left, right, "iletmp")); break;
				case TYPE_FLOAT:     push(_builder->CreateFCmpOLE(left, right,"fletmp")); break;
				case TYPE_CHARACTER: push(_builder->CreateICmpULE(left, right, "cletmp")); break;
				default: assert(false);
			}
			break;
		case TOKEN_GREATER: switch(AS_LEX(resulttype))
			{
				case TYPE_INTEGER:   push(_builder->CreateICmpSGT(left, right, "igttmp")); break;
				case TYPE_FLOAT:     push(_builder->CreateFCmpOGT(left, right,"fgttmp")); break;
				case TYPE_CHARACTER: push(_builder->CreateICmpUGT(left, right, "cgttmp")); break;
				default: assert(false);
			}
			break;
		case TOKEN_LESS: switch(AS_LEX(resulttype))
			{
				case TYPE_INTEGER:   push(_builder->CreateICmpSLT(left, right, "ilttmp")); break;
				case TYPE_FLOAT:     push(_builder->CreateFCmpOLT(left, right,"flttmp")); break;
				case TYPE_CHARACTER: push(_builder->CreateICmpULT(left, right, "clttmp")); break;
				default: assert(false);
			}
			break;

		case TOKEN_GREATER_GREATER: switch(AS_LEX(resulttype))
			{
				case TYPE_INTEGER:   push(_builder->CreateLShr(left, right, "isrtmp")); break;
				case TYPE_FLOAT:     push(_builder->CreateLShr(left, right,"fsrtmp")); break;
				case TYPE_CHARACTER: push(_builder->CreateLShr(left, right, "csrtmp")); break;
				default: assert(false);
			}
			break;
		case TOKEN_LESS_LESS: switch(AS_LEX(resulttype))
			{
				case TYPE_INTEGER:   push(_builder->CreateShl(left, right, "isltmp")); break;
				case TYPE_FLOAT:     push(_builder->CreateShl(left, right,"fsltmp")); break;
				case TYPE_CHARACTER: push(_builder->CreateShl(left, right, "csltmp")); break;
				default: assert(false);
			}
			break;

		case TOKEN_PLUS: switch(AS_LEX(resulttype))
			{
				case TYPE_INTEGER:   push(_builder->CreateAdd(left, right, "iaddtmp")); break;
				case TYPE_FLOAT:     push(_builder->CreateFAdd(left, right,"faddtmp")); break;
				case TYPE_CHARACTER: push(_builder->CreateAdd(left, right, "caddtmp")); break;
				default: assert(false);
			} 
			break;
		case TOKEN_MINUS: switch(AS_LEX(resulttype))
			{
				case TYPE_INTEGER:   push(_builder->CreateSub(left, right, "isubmp")); break;
				case TYPE_FLOAT:     push(_builder->CreateFSub(left, right,"fsubmp")); break;
				case TYPE_CHARACTER: push(_builder->CreateSub(left, right, "csubmp")); break;
				default: assert(false);
			} 
			break;
		case TOKEN_STAR: switch(AS_LEX(resulttype))
			{
				case TYPE_INTEGER:   push(_builder->CreateMul(left, right, "imultmp")); break;
				case TYPE_FLOAT:     push(_builder->CreateFMul(left, right,"fmultmp")); break;
				case TYPE_CHARACTER: push(_builder->CreateMul(left, right, "cmultmp")); break;
				default: assert(false);
			} 
			break;
		case TOKEN_SLASH: switch(AS_LEX(resulttype))
			{
				case TYPE_INTEGER:   push(_builder->CreateSDiv(left, right, "idivtmp")); break;
				case TYPE_FLOAT:     push(_builder->CreateFDiv(left, right,"fdivtmp")); break;
				case TYPE_CHARACTER: push(_builder->CreateUDiv(left, right, "cdivtmp")); break;
				default: assert(false);
			} 
			break;
		
		default: assert(false);
	}
}

VISIT(CastNode)
{
	node->_expr->accept(this);
	llvm::Value* val = pop();
	push(create_cast(val, false, node->_type->get_llvm_type(), node->_type->is_signed()));
}

VISIT(UnaryNode)
{
	node->_expr->accept(this);
	ParsedType* parsedtype = node->_expr->_cast_to;
	llvm::Type* casttype = parsedtype->get_llvm_type();
	llvm::Value* value = pop();

	// if(node->_optype != TOKEN_STAR && node->_optype != TOKEN_AND)
	// 	value = create_cast(value, false, casttype, parsedtype->is_signed());

	switch(node->_optype)
	{
		case TOKEN_STAR:
		{
			// if(casttype->isPointerTy())
				value = _builder->CreateLoad(casttype->getPointerTo(), value, "predtmp");
			push(_builder->CreateLoad(casttype, value, "dereftmp"));
			break;
		}
		case TOKEN_AND:
		{
			push(_builder->CreatePtrToInt(value, casttype, "addrtmp"));
			break;
		}
		case TOKEN_BANG:
		{
			push(_builder->CreateNot(to_bool(value)));
			break;
		}

		case TOKEN_MINUS: switch(AS_LEX(node->_expr->_cast_to))
			{
				case TYPE_INTEGER:   push(_builder->CreateNeg(value, "inegtmp")); break;
				default: assert(false);
			} 
			break;
		case TOKEN_PLUS_PLUS: switch(AS_LEX(node->_expr->_cast_to))
			{
				case TYPE_CHARACTER: push(_builder->CreateAdd(value, llvm::ConstantInt::get(casttype, 1), "cinctmp")); break;
				case TYPE_INTEGER:   push(_builder->CreateAdd(value, llvm::ConstantInt::get(casttype, 1), "iinctmp")); break;
				case TYPE_FLOAT: 	 push(_builder->CreateFAdd(value, llvm::ConstantFP::get(casttype, 1), "finctmp")); break;
				default: assert(false);
			} 
			break;
		case TOKEN_MINUS_MINUS: switch(AS_LEX(node->_expr->_cast_to))
			{
				case TYPE_CHARACTER: push(_builder->CreateSub(value, llvm::ConstantInt::get(casttype, 1), "cdectmp")); break;
				case TYPE_INTEGER:   push(_builder->CreateSub(value, llvm::ConstantInt::get(casttype, 1), "idectmp")); break;
				case TYPE_FLOAT: 	 push(_builder->CreateFSub(value, llvm::ConstantFP::get(casttype, 1), "fdectmp")); break;
				default: assert(false);
			} 
			break;
		
		default: assert(false);
	}
}

VISIT(GroupingNode)
{
	node->_expr->accept(this);
	// ParsedType* casttype = node->_cast_to;
	// push(create_cast(pop(), false, casttype->get_llvm_type(), casttype->is_signed()));
	// push(pop());
}

VISIT(SubscriptNode)
{
	node->_expr->accept(this);
	llvm::Value* ptr = pop();

	node->_index->accept(this);
	llvm::Value* index = pop();

	llvm::Value* gep = _builder->CreateInBoundsGEP(ptr, index, "sgeptmp");

	// ParsedType* casttype = node->_cast_to;
	// push(create_cast(pop(), false, casttype->get_llvm_type(), casttype->is_signed()));
	push(_builder->CreateLoad(gep, "subscrtmp"));
}


VISIT(LiteralNode)
{
	ParsedType* type = from_token_type(node->_token.type);
	llvm::Constant* constant;

	switch(node->_token.type)
	{
		case TOKEN_INTEGER:
			constant = llvm::ConstantInt::get(type->get_llvm_type(), node->_int_value, false);
			break;
		case TOKEN_CHARACTER:
			constant = llvm::ConstantInt::get(type->get_llvm_type(), node->_char_value, false);
			break;
		case TOKEN_FLOAT:
			constant = llvm::ConstantFP::get(type->get_llvm_type(), node->_float_value);
			break;
		case TOKEN_STRING:
		{
			string str = tools::escstr(node->_string_value);
			llvm::Type* chartype = _builder->getInt8Ty();

			vector<llvm::Constant *> chars(str.length());
			for(unsigned int i = 0; i < str.size(); i++)
				chars[i] = llvm::ConstantInt::get(chartype, str[i]);

			// add a zero terminator
			chars.push_back(llvm::ConstantInt::get(chartype, 0));

			// initialize the string from the characters
			llvm::ArrayType* stringtype = llvm::ArrayType::get(chartype, chars.size());

			// Create the declaration statement
			constant = _top_module->getOrInsertGlobal(".str", stringtype);
			llvm::GlobalVariable* globaldecl = (llvm::GlobalVariable*)constant;

			globaldecl->setInitializer(llvm::ConstantArray::get(stringtype, chars));
			globaldecl->setConstant(true);
			globaldecl->setLinkage(llvm::GlobalValue::LinkageTypes::PrivateLinkage);
			globaldecl->setUnnamedAddr(llvm::GlobalValue::UnnamedAddr::Global);
			globaldecl->setAlignment(llvm::MaybeAlign(1));

			constant = globaldecl;
			break;
		}
		default: assert(false);
	}

	// ParsedType* casttype = node->_cast_to;
	// push(create_cast(constant, type->is_signed(), casttype->get_llvm_type(), casttype->is_signed()));
	push(constant);
}

VISIT(ArrayNode)
{
	// alloca array
	llvm::ArrayType* arrtype = llvm::ArrayType::get(node->_cast_to->copy_element_of()->get_llvm_type(), node->_elements.size());
	llvm::AllocaInst* arr = new llvm::AllocaInst(arrtype, 0, string("arrtmp"), _builder->GetInsertBlock());
	arr->setAlignment(llvm::Align(node->_cast_to->get_alignment()));

	// get base pointer to array
	llvm::Value* idx0 = llvm::ConstantInt::get(__context, llvm::APInt(64, 0, false));
	llvm::Value* ptr = _builder->CreateInBoundsGEP(arr, (llvm::Value*[]){idx0, idx0}, "arrgeptmp");

	// iterate over elements
	for(int i = 0; i < node->_elements.size(); i++)
	{
		ExprNode* element = node->_elements[i];
		element->accept(this);
		llvm::Value* val = pop();

		_builder->CreateStore(val, ptr);
		
		// move pointer one up (so to next element)
		if(i + 1 < node->_elements.size()) ptr = _builder->CreateInBoundsGEP(
			ptr, llvm::ConstantInt::get(__context, llvm::APInt(64, 1, false)), "arrgeptmp");
	}
	
	push(_builder->CreateInBoundsGEP(arr, (llvm::Value*[]){idx0, idx0}, "arrgeptmp"));
}

VISIT(SizeOfNode)
{
	llvm::TypeSize size = _top_module->getDataLayout().getTypeAllocSize(node->_type->get_llvm_type());
	push(llvm::ConstantInt::get(node->_cast_to->get_llvm_type(), size, false));
}

VISIT(ReferenceNode)
{
	llvm::Value* var = nullptr;
	ParsedType* type = nullptr;

	if(node->_token.type == TOKEN_VARIABLE_REF)
	{
		var = _named_values[node->_variable].first;
		type = _named_values[node->_variable].second;
	}
	else if(node->_token.type == TOKEN_PARAMETER_REF)
	{
		var = _named_values[tools::fstr("%d", node->_parameter)].first;
		type = _named_values[tools::fstr("%d", node->_parameter)].second;
	}

	// cerr << string(node->_token.start, node->_token.length) + tools::fstr(" %d ", node->_token.line) + (node->_cast_to ? " yup" : " nop") << endl;

	if(type->_keep_as_reference)
	{
		// DEBUG_PRINT_F_MSG("kept '%s' as ref", node->_variable.c_str());
		push(var);
	}
	else
	{
		llvm::LoadInst* load = _builder->CreateLoad(type->get_llvm_type(), var, "loadtmp");
		// ParsedType* casttype = node->_cast_to;
		// push(create_cast(load, type->is_signed(), casttype->get_llvm_type(), casttype->is_signed()));
		push(load);
	}
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
		args.push_back(create_cast(pop(), false, casttype, node->_expected_arg_types[i]->is_signed()));
		// args.push_back(pop());
	}

	if(AS_LEX(node->_ret_type) == TYPE_VOID && !node->_ret_type->is_pointer())
		push(_builder->CreateCall(callee, args));
	else push(_builder->CreateCall(callee, args, "calltmp"));
}

#undef VISIT
#undef ERROR_AT
#undef SCOPE_UP
#undef SCOPE_DOWN