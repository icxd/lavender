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

    llvm::Value *CheckedObject::generate() {
        std::stringstream ss;
        ss << "__obj_" << m_id;
        llvm::StructType *object_type = llvm::StructType::create(s_context, ss.str());
        Vec<llvm::Type *> field_types;
        for (const CheckedField &field : m_fields)
            field_types.push_back(CheckedType(field.type()).generate());
        object_type->setBody(field_types);

        llvm::Value *object = s_builder.CreateAlloca(object_type);
        for (const CheckedField &field : m_fields) {
            if (not field.value().has_value()) continue;
            llvm::Value *value = field.value().value()->generate();
            if (not value)
                PANIC("COMPILER ERROR: CHECKER RETURNED NULLPTR FOR FIELD VALUE");
            s_builder.CreateStore(value, s_builder.CreateStructGEP(object_type, object, 0));
        }
        return object;
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
