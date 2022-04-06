#include "debug.hpp"
#include <unistd.h>

DebugInfoBuilder::DebugInfoBuilder(llvm::Module* module, IRBUILDER builder, string filepath, bool optimized)
{
	// get current directory
	_directory = string(getcwd(nullptr, 0));

	// create the builder and compile unit
	_debug_builder = make_unique<llvm::DIBuilder>(*module);

	_cunit = _debug_builder->createCompileUnit(
		llvm::dwarf::DW_LANG_C, // closest thing to Evi
		create_file_unit(filepath),
		APP_NAME_INTERNAL, optimized, "", 0
	);

	// current debug info version
	module->addModuleFlag(llvm::Module::Warning, "Debug Info Version", llvm::DEBUG_METADATA_VERSION);

	// Darwin only supports dwarf2.
	if(llvm::Triple(llvm::sys::getProcessTriple()).isOSDarwin())
		module->addModuleFlag(llvm::Module::Warning, "Dwarf Version", 2);

	_ir_builder = builder;
}

void DebugInfoBuilder::finish()
{
	// finalize the DIBuilder
	_debug_builder->finalize();
	DEBUG_PRINT_MSG("Debug info builder finalized.");
}

// ===================================================

llvm::DIFile* DebugInfoBuilder::create_file_unit(string filename)
{
	// just create the file and return it
	return _debug_builder->createFile(filename, _directory);
}

llvm::DISubprogram* DebugInfoBuilder::create_subprogram(FuncDeclNode* node, llvm::DIFile* file_unit)
{
	llvm::DISubprogram* subprog = _debug_builder->createFunction(
		file_unit, node->_identifier, node->_identifier, // llvm::StringRef(),
		file_unit, node->_token.line, get_function_type(node, file_unit), node->_token.line,
		llvm::DINode::FlagPrototyped, llvm::DISubprogram::SPFlagDefinition);

	return subprog;
}

// ===================================================

void DebugInfoBuilder::emit_location(uint line, uint col)
{
	llvm::DIScope *scope = _lexical_blocks.empty() ? _cunit : _lexical_blocks.back();
	_ir_builder->SetCurrentDebugLocation(llvm::DILocation::get(scope->getContext(), line, col, scope));
}

void DebugInfoBuilder::push_subprogram(llvm::DISubprogram* sub_program)
{
	_lexical_blocks.push_back(sub_program);
	emit_location(0, 0);
}

// ===================================================

llvm::DIType* DebugInfoBuilder::get_type(ParsedType* type)
{
	if(type->is_pointer()) 
	{
		llvm::DIType* elementtype = get_type(type->copy_element_of());
		return _debug_builder->createPointerType(elementtype, elementtype->getSizeInBits());
	}
	
	DEBUG_PRINT_F_MSG("Getting debug type for '%s'.", type->to_c_string());

	// get size in bits
	uint64_t size = 0; // _ir_builder->GetInsertPoint()->getModule()->getDataLayout()
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
	return _debug_builder->createBasicType(type->to_string(), size, encoding);

}

llvm::DISubroutineType* DebugInfoBuilder::get_function_type(FuncDeclNode* node, llvm::DIFile* file_unit)
{
	vector<llvm::Metadata*> types;

	types.push_back(get_type(node->_ret_type));

	for (int i = 0; i < node->_params.size(); i++)
		types.push_back(get_type(node->_params[i]));

	return _debug_builder->createSubroutineType(_debug_builder->getOrCreateTypeArray(types));
}