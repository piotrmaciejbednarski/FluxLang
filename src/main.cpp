#include "common/arena.h"
#include "common/source.h"
#include "common/error.h"
#include "lexer/tokenizer.h"
#include "output/writer.h"
#include <iostream>
#include <string>
#include <iomanip>

using namespace flux;

void printUsage(const char* programName) {
    std::cout << "Flux Lexer\n";
    std::cout << "Usage: " << programName << " <file.fx>\n\n";
    std::cout << "This will tokenize the Flux source file and display all tokens.\n";
}

int main(int argc, char* argv[]) {
    // Check arguments
    if (argc != 2) {
        if (argc > 1 && (std::string(argv[1]) == "--help" || std::string(argv[1]) == "-h")) {
            printUsage(argv[0]);
            return 0;
        }
        std::cerr << "Error: Please provide a Flux source file\n";
        printUsage(argv[0]);
        return 1;
    }
    
    std::string filename = argv[1];
    
    try {
        // Create arena for memory management
        common::Arena arena;
        
        // Load source file
        output::defaultWriter.info("Loading file: " + filename);
        auto source = common::Source::fromFile(filename, arena);
        
        output::defaultWriter.info("File loaded successfully");
        output::defaultWriter.info("Size: " + std::to_string(source->text().size()) + " bytes");
        output::defaultWriter.info("Lines: " + std::to_string(source->lineCount()));
        std::cout << "\n";
        
        // Create tokenizer
        lexer::Tokenizer tokenizer(source, arena);
        
        // Tokenize
        output::defaultWriter.info("Starting lexical analysis...");
        auto tokens = tokenizer.tokenize();
        
        // Check for errors
        if (tokenizer.errorCollector().hasErrors()) {
            output::defaultWriter.error("Lexical analysis failed with errors:");
            tokenizer.errorCollector().reportErrors();
            return 1;
        }
        
        output::defaultWriter.info("Lexical analysis completed successfully");
        output::defaultWriter.info("Tokens found: " + std::to_string(tokens.size()));
        std::cout << "\n";
        
        // Print tokens in a formatted table
        std::cout << "TOKEN LIST\n";
        std::cout << "==========\n";
        std::cout << std::left << std::setw(4) << "#" 
                  << std::setw(20) << "Type" 
                  << std::setw(15) << "Position" 
                  << std::setw(30) << "Text" 
                  << "Value\n";
        std::cout << std::string(80, '-') << "\n";
        
        for (size_t i = 0; i < tokens.size(); i++) {
            const auto& token = tokens[i];
            
            // Format position
            std::string position = std::to_string(token.range.start.line) + ":" + 
                                 std::to_string(token.range.start.column);
            
            // Format text (escape special characters and limit length)
            std::string text = std::string(token.text);
            if (text.length() > 25) {
                text = text.substr(0, 22) + "...";
            }
            
            // Replace special characters for display
            for (char& c : text) {
                if (c == '\n') c = ' ';
                if (c == '\t') c = ' ';
                if (c == '\r') c = ' ';
            }
            
            // Format additional value information
            std::string value = "";
            if (token.type == lexer::TokenType::INTEGER_LITERAL) {
                value = "(" + std::to_string(token.intValue) + ")";
            } else if (token.type == lexer::TokenType::FLOAT_LITERAL) {
                value = "(" + std::to_string(token.floatValue) + ")";
            }
            
            std::cout << std::left << std::setw(4) << i + 1
                      << std::setw(20) << lexer::tokenTypeToString(token.type)
                      << std::setw(15) << position
                      << std::setw(30) << ("\"" + text + "\"")
                      << value << "\n";
        }
        
        std::cout << std::string(80, '-') << "\n";
        std::cout << "Total tokens: " << tokens.size() << "\n";
        
        // Print summary statistics
        std::cout << "\nTOKEN STATISTICS\n";
        std::cout << "================\n";
        
        size_t identifiers = 0, keywords = 0, operators = 0, literals = 0, punctuation = 0;
        
        for (const auto& token : tokens) {
            if (token.type == lexer::TokenType::IDENTIFIER) {
                identifiers++;
            } else if (lexer::isKeyword(token.type)) {
                keywords++;
            } else if (lexer::isOperator(token.type)) {
                operators++;
            } else if (lexer::isLiteral(token.type)) {
                literals++;
            } else if (token.type != lexer::TokenType::END_OF_FILE) {
                punctuation++;
            }
        }
        
        std::cout << "Identifiers: " << identifiers << "\n";
        std::cout << "Keywords: " << keywords << "\n";
        std::cout << "Operators: " << operators << "\n";
        std::cout << "Literals: " << literals << "\n";
        std::cout << "Punctuation: " << punctuation << "\n";
        
        output::defaultWriter.info("\nLexical analysis completed successfully!");
        
    } catch (const common::Error& e) {
        output::defaultWriter.error("Flux error: " + e.message());
        e.report();
        return 1;
    } catch (const std::exception& e) {
        output::defaultWriter.error("System error: " + std::string(e.what()));
        return 1;
    }
    
    return 0;
}