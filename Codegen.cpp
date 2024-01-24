#include "Common.hpp"
#include "Codegen.hpp"
#include "Checker.hpp"
#include <iostream>
#include <sstream>

llvm::Type *CheckedType::generate() {
    switch (m_type.type) {
        case Type::Kind::Int: return llvm::Type::getInt32Ty(s_context);
        case Type::Kind::Str: return llvm::Type::getInt8PtrTy(s_context);
        case Type::Kind::Id: {
            llvm::Value *id = s_named_values[m_type.id.value];
            if (not id)
                PANIC("COMPILER ERROR: id `%s` USED BUT NOT DEFINED FOR CODEGEN", m_type.id.value.c_str());
            return id->getType();
        }
        case Type::Kind::Array: {
            // TODO: don't represent dynamic arrays as pointers.
            llvm::Type *element_type = CheckedType(*m_type.subtype).generate();
            return llvm::PointerType::get(element_type, 0);
        }
    }
}

namespace CheckedStmtDetails {

    llvm::Constant *CheckedObject::generate() {
        llvm::StructType *object_type = llvm::StructType::create(s_context, m_id);
        llvm::PointerType *object_ptr_type = llvm::PointerType::get(object_type, 0);

        Vec<llvm::Type *> elements{object_ptr_type};

        for (auto &field : m_fields) {
            llvm::Type *field_type = CheckedType(field.type()).generate();
            elements.push_back(field_type);
        }

        object_type->setBody(elements);

        // `new` method for the object.
        llvm::FunctionType *new_type = llvm::FunctionType::get(object_ptr_type, false);
        llvm::Function *new_fun = llvm::Function::Create(new_type, llvm::Function::ExternalLinkage, "new", s_module.get());
        llvm::BasicBlock *new_block = llvm::BasicBlock::Create(s_context, "entry", new_fun);
        s_builder.SetInsertPoint(new_block);

        llvm::Value *object = s_builder.CreateAlloca(object_type);
        s_builder.CreateRet(object);

        // `init` method for the object.
        llvm::FunctionType *init_type = llvm::FunctionType::get(llvm::Type::getVoidTy(s_context), false);
        llvm::Function *init_fun = llvm::Function::Create(init_type, llvm::Function::ExternalLinkage, "init", s_module.get());
        llvm::BasicBlock *init_block = llvm::BasicBlock::Create(s_context, "entry", init_fun);
        s_builder.SetInsertPoint(init_block);

        return llvm::ConstantStruct::get(object_type, llvm::ArrayRef<llvm::Constant *>());
    }

    llvm::Function *CheckedFun::generate() { UNIMPLEMENTED; }

}

namespace CheckedExprDetails {

    llvm::Value *CheckedId::generate() {
        llvm::Value *id = s_named_values[m_id];
        if (not id)
            PANIC("COMPILER ERROR: id `%s` USED BUT NOT DEFINED FOR CODEGEN", m_id.c_str());
        return id;
    }

    llvm::Value *CheckedInt::generate() { return llvm::ConstantInt::get(s_context, llvm::APInt(32, m_value)); }

    llvm::Value *CheckedString::generate() {
        std::stringstream ss;
        ss << "__str_" << str_count;
        return s_builder.CreateGlobalString(m_value, ss.str(), 0, s_module.get());
    }

    llvm::Value *CheckedCall::generate() {
        UNIMPLEMENTED;
    }

}
