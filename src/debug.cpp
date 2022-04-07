#include "debug.hpp"
#include <unistd.h>

DebugInfoBuilder::DebugInfoBuilder(llvm::Module* module, IRBUILDER builder, string filepath, bool optimized)
{
	// get current directory
	_directory = string(getcwd(nullptr, 0));

	// create the builder and compile unit
	_dbuilder = make_unique<llvm::DIBuilder>(*module);

	_cunit = _dbuilder->createCompileUnit(
		llvm::dwarf::DW_LANG_C, // closest thing to Evi
		create_file_unit(filepath),
		APP_NAME_INTERNAL, optimized, "", 0
	);

	// current debug info version
	module->addModuleFlag(llvm::Module::Warning, "Debug Info Version", llvm::DEBUG_METADATA_VERSION);

	// Darwin only supports dwarf2.
	if(llvm::Triple(llvm::sys::getProcessTriple()).isOSDarwin())
		module->addModuleFlag(llvm::Module::Warning, "Dwarf Version", 2);

	_ibuilder = builder;
}

void DebugInfoBuilder::finish()
{
	// finalize the DIBuilder
	_dbuilder->finalize();
	DEBUG_PRINT_MSG("Debug info builder finalized.");
}

// ===================================================

llvm::DIFile* DebugInfoBuilder::create_file_unit(string filename)
{
	// just create the file and return it
	return _dbuilder->createFile(filename, _directory);
}

llvm::DISubprogram* DebugInfoBuilder::create_subprogram(FuncDeclNode* node, llvm::DIFile* file_unit)
{
	// llvm::DITemplateParameterArray params = llvm::DITemplateParameterArray();

	llvm::DISubprogram* subprog = _dbuilder->createFunction(
		file_unit, node->_identifier, node->_identifier,
		file_unit, node->_token.line, get_function_type(node, file_unit), node->_token.line,
		llvm::DINode::FlagPrototyped, llvm::DISubprogram::SPFlagDefinition); //, params);

	return subprog;
}

llvm::DILocalVariable* DebugInfoBuilder::create_variable(string ident, int line_no, ParsedType* type, bool global)
{
	string name = '$' + ident;
	llvm::DIScope* scope = _scopes.back();

	return _dbuilder->createAutoVariable(scope, name, scope->getFile(), line_no, get_type(type));
}

llvm::DILocalVariable* DebugInfoBuilder::create_parameter(int index, int line_no, ParsedType* type)
{
	string name = '$' + to_string(index);
	llvm::DIScope* scope = _scopes.back();

	return _dbuilder->createParameterVariable(scope, name, index, 
		scope->getFile(), line_no, get_type(type));
}

// ===================================================

void DebugInfoBuilder::emit_location(uint line, uint col)
{
	llvm::DIScope *scope = _scopes.empty() ? _cunit : _scopes.back();
	_ibuilder->SetCurrentDebugLocation(llvm::DILocation::get(scope->getContext(), line, col, scope));
}

void DebugInfoBuilder::insert_declare(llvm::Value* storage, llvm::DILocalVariable* varinfo)
{
	_dbuilder->insertDeclare(storage, varinfo,
		_dbuilder->createExpression(),
		_ibuilder->getCurrentDebugLocation(),
		_ibuilder->GetInsertBlock()
	);
}

void DebugInfoBuilder::push_subprogram(llvm::DISubprogram* sub_program)
{
	_scopes.push_back(sub_program);
	emit_location(0, 0);
}

void DebugInfoBuilder::pop_subprogram()
{
	// just a lil pop
	_scopes.pop_back();
}

// ===================================================

llvm::DIType* DebugInfoBuilder::get_type(ParsedType* type)
{
	if(type->is_pointer()) 
	{
		llvm::DIType* elementtype = get_type(type->copy_element_of());
		return _dbuilder->createPointerType(elementtype, elementtype->getSizeInBits());
	}
	
	// get size in bits
	uint64_t size = type->get_alignment() * 8;
	// uint64_t size = _ir_builder->GetInsertPoint()->getModule()->getDataLayout()
					  // .getTypeAllocSize(type->get_llvm_type()).getFixedSize();

	// get encoding
	llvm::dwarf::TypeKind encoding = (llvm::dwarf::TypeKind)0;
	switch(type->_lexical_type)
	{
		case TYPE_BOOL: 	 encoding = llvm::dwarf::DW_ATE_boolean; break;
		case TYPE_CHARACTER: encoding = llvm::dwarf::DW_ATE_unsigned_char; break;
		case TYPE_FLOAT: 	 encoding = llvm::dwarf::DW_ATE_float; break;
		case TYPE_VOID: 	 encoding = (llvm::dwarf::TypeKind)0; break;
		case TYPE_INTEGER:	 encoding = type->is_signed() ? 
										llvm::dwarf::DW_ATE_signed :
										llvm::dwarf::DW_ATE_unsigned; break;
		default: THROW_INTERNAL_ERROR("during debuging info generation");
	}

	// create and return type
	return _dbuilder->createBasicType(type->to_string(), size, encoding);

}

llvm::DISubroutineType* DebugInfoBuilder::get_function_type(FuncDeclNode* node, llvm::DIFile* file_unit)
{
	vector<llvm::Metadata*> types;

	types.push_back(get_type(node->_ret_type));

	for (int i = 0; i < node->_params.size(); i++)
		types.push_back(get_type(node->_params[i]));

	return _dbuilder->createSubroutineType(_dbuilder->getOrCreateTypeArray(types));
}