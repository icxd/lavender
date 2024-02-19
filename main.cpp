#include "Common.hpp"
#include "Checker.hpp"
#include "Parser.hpp"
#include "Token.hpp"
#include "Tokenizer.hpp"
#include <fstream>
#include <iostream>
#include <sstream>

void display_error(const Error &, const Str &);

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

    Project project{};

    auto tokenize_result = tokenize(filename, source);

    for (auto &error : tokenize_result.errors) {
        display_error(error, source);
    }
    if (!tokenize_result.errors.empty()) return 1;

    auto tokens = normalize(tokenize_result.tokens);

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
    ErrorOr<Vec<ParsedStatement *>> stmts = parser.parse();
    if (not stmts.has_value()) {
        Error error = stmts.error();
        display_error(error, source);
        return 1;
    }
    auto statements = stmts.value();

    AstPrinter printer{};
    printer.print(statements);

    auto *scope = new Scope(std::make_optional(0));
    project.scopes.push_back(scope);
    ScopeId scope_id = project.scopes.size() - 1;

    Opt<Error> result = typecheck_namespace(parser.parsed_namespace(), scope_id, project);
    if (result.has_value()) {
        Error error = result.value();
        display_error(error, source);
        return 1;
    }

    return 0;
}

void display_error(const Error &error, const Str &source) {
    auto &span = error.span;

    std::cout << "\033[1;1m" << span.filename << ":" << span.line << ":"
              << span.column << ": \033[31;1merror: \033[0m" << error.message
              << "\n";

    Vec<Str> lines = split(source, '\n');
    Str line = lines[span.line - 1];

    // TODO: highlight snippet using `tokenize(span.filename, source);`
    usz length = std::to_string(span.line).size();
    std::cout << "\033[36;1m " << span.line << " | \033[0m" << line << "\n";
    std::cout << "\033[36;1m " << std::string(length, ' ') << " | \033[31;1m" << std::string(span.column - 1, ' ')
              << '^' << std::string(span.length - 1, '~') << '\n';
}