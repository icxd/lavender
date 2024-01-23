#include "Common.hpp"
#include "Checker.hpp"
#include "Parser.hpp"
#include "Token.hpp"
#include "Tokenizer.hpp"
#include "Codegen.hpp"
#include <fstream>
#include <iostream>
#include <sstream>

extern Unique<llvm::Module> s_module;

int main(int argc, char *argv[]) {
    if (argc < 2) {
        return 1;
    }

    auto filename = argv[1];
    std::fstream file(filename);
    if (!file.is_open()) {
        std::cout << "error: could not open file `" << filename << "`\n";
        return 1;
    }

    std::stringstream ss;
    ss << file.rdbuf();
    auto source = ss.str();

    auto result = tokenize(filename, source);

    for (auto &error : result.errors) {
        auto &span = error.span;

        std::cout << "\033[1m" << span.filename << ":" << span.line << ":"
                  << span.column << ": \033[31merror: \033[0m" << error.message
                  << "\n";
    }
    if (!result.errors.empty()) return 1;

    auto tokens = normalize(result.tokens);

    for (auto &token : tokens) {
        auto &span = token.span;

        std::cout << "\033[1m" << span.filename << ":" << span.line << ":"
                  << span.column << ": \033[32mtoken: \033[0m"
                  << Token::type_to_string(token.type);
        switch (token.type) {
        case Token::Type::Id:
        case Token::Type::Int:
        case Token::Type::Float:
        case Token::Type::String:
            std::cout << " `" << token.value.value() << "`";
            break;
        default:
            break;
        }
        std::cout << "\n";
    }

    Parser parser(tokens);
    ErrorOr<Vec<Stmt *>> stmts = parser.parse();
    if (not stmts.has_value()) {
        Error error = stmts.error();
        auto &span = error.span;

        std::cout << "\033[1m" << span.filename << ":" << span.line << ":"
                  << span.column << ": \033[31merror: \033[0m" << error.message
                  << "\n";
        return 1;
    }
    auto statements = stmts.value();

    AstPrinter printer{};
    printer.print(statements);

    Checker checker;
    ErrorOr<Vec<CheckedStmt *>> checker_result = checker.check(statements);
    if (not stmts.has_value()) {
        Error error = stmts.error();
        auto &span = error.span;

        std::cout << "\033[1m" << span.filename << ":" << span.line << ":"
                  << span.column << ": \033[31merror: \033[0m" << error.message
                  << "\n";
        return 1;
    }

    s_module = std::make_unique<llvm::Module>("main", s_context);

    for (auto &stmt : checker_result.value()) {
        switch (stmt->type()) {
        case CheckedStmt::Type::Object: {
            auto object = dynamic_cast<CheckedStmtDetails::CheckedObject *>(stmt)->generate();
            s_module->getOrInsertGlobal(object->getName(), object->getType());
        } break;
        case CheckedStmt::Type::Fun: {
        } break;
        }
    }

    s_module->print(llvm::outs(), nullptr);

    return 0;
}