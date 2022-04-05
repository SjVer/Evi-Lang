#include "debug.hpp"
#include <unistd.h>

DebugInfoBuilder::DebugInfoBuilder(llvm::Module* module, string filepath, bool optimized)
{
	// create the builder and compile unit
	_builder = make_unique<llvm::DIBuilder>(*module);

	_cunit = _builder->createCompileUnit(
		llvm::dwarf::DW_LANG_C, // closest thing to Evi
		_builder->createFile(filepath, string(getcwd(nullptr, 0))),
		APP_NAME_INTERNAL, optimized, "", 0
	);

	// current debug info version
	module->addModuleFlag(llvm::Module::Warning, "Debug Info Version", llvm::DEBUG_METADATA_VERSION);

	// Darwin only supports dwarf2.
	if(llvm::Triple(llvm::sys::getProcessTriple()).isOSDarwin())
		module->addModuleFlag(llvm::Module::Warning, "Dwarf Version", 2);
}

void DebugInfoBuilder::finish()
{
	// finalize the DIBuilder
	_builder->finalize();
	DEBUG_PRINT_MSG("Debug info builder finalized.");
}

// ===================================================

void DebugInfoBuilder::emit_location(uint line)
{
	llvm::DIScope *scope = LexicalBlocks.empty() ? _cunit : LexicalBlocks.back();
	_builder->SetCurrentDebugLocation(llvm::DILocation::get(scope->getContext(), line, 0, scope));
}