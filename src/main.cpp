#include <iostream>
#include <string>
#include <memory>
#include <cstring>
#include <iomanip>

// Include all necessary headers
#include "common/arena.h"
#include "common/source.h"
#include "common/error.h"
#include "output/writer.h"
#include "lexer/tokenizer.h"
#include "lexer/token.h"
#include "parser/parser.h"
#include "parser/ast.h"

using namespace flux;

void printUsage(const char* program_name) {
    std::cout << "Usage: " << program_name << " [OPTIONS] <file>\n";
    std::cout << "Options:\n";
    std::cout << "  -l    Lex the file (tokenize only)\n";
    std::cout << "  -p    Parse the file (build AST)\n";
    std::cout << "  -h    Show this help message\n";
    std::cout << "\nExamples:\n";
    std::cout << "  " << program_name << " -l ../example.fx\n";
    std::cout << "  " << program_name << " -p ../example.fx\n";
}

int lexFile(const std::string& filename) {
    try {
        // Create arena and source
        common::Arena arena;
        auto source = common::Source::fromFile(filename, arena);
        
        // Create tokenizer
        lexer::Tokenizer tokenizer(source, arena);
        
        // Tokenize and print tokens
        auto tokens = tokenizer.tokenizeAll();
        
        std::cout << "=== TOKENIZATION RESULTS ===\n";
        std::cout << "File: " << filename << "\n";
        std::cout << "Tokens found: " << tokens.size() << "\n\n";
        
        for (const auto& token : tokens) {
            std::cout << std::left << std::setw(20) << token.typeToString() 
                      << " | " << std::setw(15) << token.text 
                      << " | Line " << token.position.line 
                      << ", Col " << token.position.column;
            
            // Print processed text for literals if different from raw text
            if (!token.processed_text.empty() && token.processed_text != token.text) {
                std::cout << " | Processed: \"" << token.processed_text << "\"";
            }
            
            // Print values for numeric literals
            if (token.type == lexer::TokenType::INTEGER_LITERAL) {
                std::cout << " | Value: " << token.integer_value;
            } else if (token.type == lexer::TokenType::FLOAT_LITERAL) {
                std::cout << " | Value: " << token.float_value;
            }
            
            std::cout << "\n";
        }
        
        // Report any tokenization errors
        if (tokenizer.hasErrors()) {
            std::cout << "\n=== LEXER ERRORS ===\n";
            tokenizer.getErrors().reportErrors();
            return 1;
        }
        
        std::cout << "\nTokenization completed successfully.\n";
        return 0;
        
    } catch (const common::Error& e) {
        std::cerr << "Lexer error: " << e.what() << std::endl;
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}

int parseFile(const std::string& filename) {
    try {
        // Create arena and source
        common::Arena arena;
        auto source = common::Source::fromFile(filename, arena);
        
        // Create parser
        parser::Parser parser(source, arena);
        
        // Parse the program
        auto program = parser.parseProgram();
        
        std::cout << "=== PARSING RESULTS ===\n";
        std::cout << "File: " << filename << "\n";
        
        if (program) {
            std::cout << "Global declarations found: " << program->getDeclarations().size() << "\n";
            
            // Count different types of declarations
            int imports = 0, namespaces = 0, objects = 0, structs = 0, functions = 0, variables = 0, templates = 0, operators = 0, usings = 0;
            
            for (const auto* decl : program->getDeclarations()) {
                if (decl->as<parser::ImportDeclaration>()) imports++;
                else if (decl->as<parser::UsingDeclaration>()) usings++;
                else if (decl->as<parser::NamespaceDeclaration>()) namespaces++;
                else if (decl->as<parser::ObjectDeclaration>()) objects++;
                else if (decl->as<parser::StructDeclaration>()) structs++;
                else if (decl->as<parser::FunctionDeclaration>()) functions++;
                else if (decl->as<parser::VariableDeclaration>()) variables++;
                else if (decl->as<parser::TemplateDeclaration>()) templates++;
                else if (decl->as<parser::OperatorDeclaration>()) operators++;
            }
            
            std::cout << "  - Import statements: " << imports << "\n";
            std::cout << "  - Using statements: " << usings << "\n";
            std::cout << "  - Namespaces: " << namespaces << "\n";
            std::cout << "  - Objects: " << objects << "\n";
            std::cout << "  - Structs: " << structs << "\n";
            std::cout << "  - Functions: " << functions << "\n";
            std::cout << "  - Variables: " << variables << "\n";
            std::cout << "  - Templates: " << templates << "\n";
            std::cout << "  - Operators: " << operators << "\n";
        } else {
            std::cout << "Failed to parse program.\n";
        }
        
        // Report any parsing errors
        if (parser.hasErrors()) {
            std::cout << "\n=== PARSER ERRORS ===\n";
            parser.getErrors().reportErrors();
            return 1;
        }
        
        std::cout << "\nParsing completed successfully.\n";
        return 0;
        
    } catch (const common::Error& e) {
        std::cerr << "Parser error: " << e.what() << std::endl;
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}

int main(int argc, char* argv[]) {
    // Check for minimum arguments
    if (argc < 2) {
        printUsage(argv[0]);
        return 1;
    }
    
    // Parse command line arguments
    std::string mode;
    std::string filename;
    
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        
        if (arg == "-h" || arg == "--help") {
            printUsage(argv[0]);
            return 0;
        } else if (arg == "-l") {
            mode = "lex";
        } else if (arg == "-p") {
            mode = "parse";
        } else if (arg[0] == '-') {
            std::cerr << "Unknown option: " << arg << std::endl;
            printUsage(argv[0]);
            return 1;
        } else {
            // This should be the filename
            filename = arg;
        }
    }
    
    // Validate arguments
    if (mode.empty()) {
        std::cerr << "Error: Must specify either -l (lex) or -p (parse)" << std::endl;
        printUsage(argv[0]);
        return 1;
    }
    
    if (filename.empty()) {
        std::cerr << "Error: Must specify input file" << std::endl;
        printUsage(argv[0]);
        return 1;
    }
    
    // Execute the requested operation
    if (mode == "lex") {
        return lexFile(filename);
    } else if (mode == "parse") {
        return parseFile(filename);
    }
    
    return 0;
}