#pragma once

#include <utility>

#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"

static llvm::LLVMContext s_context;
static llvm::IRBuilder<> s_builder(s_context);
static Unique<llvm::Module> s_module;
static Map<Str, llvm::Value *> s_named_values;
static usz str_count = 0;
