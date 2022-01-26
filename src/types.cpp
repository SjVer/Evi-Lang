#include "types.hpp"

const char* lexical_type_strings[__TYPE_NONE];

map<string, EviType> __evi_types;
llvm::LLVMContext __context;