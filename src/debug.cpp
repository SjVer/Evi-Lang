#include "debug.hpp"

DebugInfo::DebugInfo(llvm::Module* module, const char* filename)
{
	_builder = make_unique<llvm::DIBuilder<>>(*module);

	_cunit = _builder->createCompileUnit(
		llvm::dwarf::DW_LANG_C, // closest thing to Evi
		_builder->createFile(filename, "."),
		APP_NAME_INTERNAL, false, "", 0
	);

	// current debug info version
	module->addModuleFlag(llvm::Module::Warning, "Debug Info Version", DEBUG_METADATA_VERSION);

	// Darwin only supports dwarf2.
	if(llvm::Triple(llvm::sys::getProcessTriple()).isOSDarwin())
		module->addModuleFlag(llvm::Module::Warning, "Dwarf Version", 2);
}

void DebugInfo::finish()
{
	// finalize the DIBuilder
	_builder->finalize();
}