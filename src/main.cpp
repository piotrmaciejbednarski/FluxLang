#include <iostream>
#include <fstream>
#include <string>
#include <memory>
#include <vector>
#include <unordered_map>

#include "common/arena.h"
#include "common/error.h"
#include "common/source.h"
#include "lexer/token.h"
#include "lexer/tokenizer.h"
#include "parser/parser.h"
#include "output/writer.h"

// Print program usage
void printUsage(const std::string& program_name) {
    std::cout << "Usage: " << program_name << " [options] <flux_file>" << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  --l             Lex only (tokenize the input file)" << std::endl;
    std::cout << "  --p             Parse only (create AST but don't execute)" << std::endl;
    std::cout << "  -e \"<flux_code>\" Execute code directly from command line" << std::endl;
    std::cout << "  -h, --help      Display this help message" << std::endl;
}

// Process source for lexing only
void processLexing(const std::shared_ptr<flux::common::Source>& source) {
    // Create tokenizer
    flux::lexer::Tokenizer tokenizer(source);
    
    // Tokenize all
    std::vector<flux::lexer::Token> tokens = tokenizer.tokenizeAll();
    
    // Print tokens
    std::cout << "Tokenization results:" << std::endl;
    std::cout << "---------------------" << std::endl;
    
    for (const auto& token : tokens) {
        std::cout << token.toString() << std::endl;
        
        // Stop if we hit an error
        if (token.type() == flux::lexer::TokenType::ERROR) {
            break;
        }
    }
    
    // Report any errors
    if (tokenizer.hasErrors()) {
        std::cout << "\nErrors:" << std::endl;
        std::cout << "-------" << std::endl;
        tokenizer.errors().reportErrors();
    } else {
        std::cout << "\nTokenization completed successfully." << std::endl;
    }
    
    // Print token statistics
    std::cout << "\nStatistics:" << std::endl;
    std::cout << "----------" << std::endl;
    std::cout << "Total tokens: " << tokens.size() << std::endl;
    
    // Count token types
    std::unordered_map<flux::lexer::TokenType, int> token_counts;
    for (const auto& token : tokens) {
        token_counts[token.type()]++;
    }
    
    // Print token type counts
    std::cout << "Token type distribution:" << std::endl;
    for (const auto& [type, count] : token_counts) {
        std::cout << "  " << flux::lexer::Token::tokenTypeToString(type) << ": " << count << std::endl;
    }
}

// Process source for parsing only
void processParsing(const std::shared_ptr<flux::common::Source>& source) {
    // Create tokenizer
    flux::lexer::Tokenizer tokenizer(source);
    
    // Create parser
    flux::parser::Parser parser(tokenizer);
    
    // Parse the program
    std::unique_ptr<flux::parser::Program> program = parser.parseProgram();
    
    // Report success or errors
    if (parser.hasErrors()) {
        std::cout << "Parsing failed with errors:" << std::endl;
        std::cout << "-------------------------" << std::endl;
        parser.errors().reportErrors();
    } else {
        std::cout << "Parsing completed successfully." << std::endl;
        std::cout << "Number of top-level declarations: " << program->declarations.size() << std::endl;
        
        // TODO: Add more information about the parsed program when needed
    }
}

// Process source (default includes both lexing and parsing)
void processSource(const std::shared_ptr<flux::common::Source>& source, bool lexOnly, bool parseOnly) {
    try {
        if (lexOnly) {
            processLexing(source);
        } else if (parseOnly) {
            processParsing(source);
        } else {
            // Default: execute the program (for now, just parse it)
            processParsing(source);
            
            // TODO: Add type checking and execution steps here
            std::cout << "\nExecution is not yet implemented." << std::endl;
        }
    } catch (const flux::common::Error& e) {
        // Report error
        e.report();
    }
}

int main(int argc, char* argv[]) {
    try {
        // Process command line arguments
        if (argc < 2) {
            printUsage(argv[0]);
            return 1;
        }
        
        // Check for help flag
        std::string arg1(argv[1]);
        if (arg1 == "-h" || arg1 == "--help") {
            printUsage(argv[0]);
            return 0;
        }
        
        // Initialize arena
        flux::common::Arena arena;
        
        // Parse options
        bool lexOnly = false;
        bool parseOnly = false;
        std::string filename;
        
        for (int i = 1; i < argc; ++i) {
            std::string arg(argv[i]);
            
            if (arg == "--l") {
                lexOnly = true;
            } else if (arg == "--p") {
                parseOnly = true;
            } else if (arg == "-e" && i + 1 < argc) {
                // Process from command line
                std::string code(argv[++i]);
                auto source = flux::common::Source::fromString(code, "<command_line>", arena);
                processSource(source, lexOnly, parseOnly);
                return 0;
            } else if (arg[0] != '-') {
                // Assume it's the filename
                filename = arg;
            } else {
                std::cerr << "Unknown option: " << arg << std::endl;
                printUsage(argv[0]);
                return 1;
            }
        }
        
        if (filename.empty()) {
            std::cerr << "No input file specified." << std::endl;
            printUsage(argv[0]);
            return 1;
        }
        
        // Process from file
        auto source = flux::common::Source::fromFile(filename, arena);
        processSource(source, lexOnly, parseOnly);
        
        return 0;
    } catch (const flux::common::Error& e) {
        // Report error
        e.report();
        return 1;
    } catch (const std::exception& e) {
        // Report standard exception
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        // Report unknown exception
        std::cerr << "Unknown error occurred" << std::endl;
        return 1;
    }
}