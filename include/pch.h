#include "llvm/Config/llvm-config.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Function.h"
#include "llvm/Transforms/Utils/ModuleUtils.h"
// #include "llvm/LineEditor/LineEditor.h"
#include "llvm/Support/raw_os_ostream.h"

#include "llvm/IR/Verifier.h"
#include <llvm/IR/NoFolder.h>

#include <string>
#include <cassert>
#include <iostream>
#include <fstream>
#include <sstream>