#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include "include/lexer.h"
#include "include/parser.h"

using namespace flux;

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <source_file>" << std::endl;
        return 1;
    }

    // Read source file
    std::string filename = argv[1];
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open file '" << filename << "'" << std::endl;
        return 1;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string source = buffer.str();
    file.close();

    // Create lexer and tokenize the source
    Lexer lexer(source, filename);
    
    // Collect all tokens
    std::vector<Token> tokens;
    
    // Output header with the filename
    std::cout << "====================================================" << std::endl;
    std::cout << "Lexical Analysis for " << filename << std::endl;
    std::cout << "====================================================" << std::endl;
    
    // Print the table header
    Lexer::printTokenTableHeader();
    
    // Process all tokens
    Token token = lexer.nextToken();
    while (token.type != TokenType::END_OF_FILE) {
        if (token.type == TokenType::ERROR) {
            std::cerr << "Error at line " << token.line << ", column " << token.column
                     << ": " << std::get<std::string>(token.value.value) << std::endl;
            return 1;
        }
        
        // Print the token information
        Lexer::printTokenTableRow(token);
        tokens.push_back(token);
        
        token = lexer.nextToken();
    }
    
    // Print the EOF token
    Lexer::printTokenTableRow(token);
    tokens.push_back(token);
              
    std::cout << "====================================================" << std::endl;
    std::cout << "Lexical analysis completed successfully." << std::endl;
    std::cout << "====================================================" << std::endl;
    
    // Parse the tokens
	try {
	    Parser parser(tokens);
	    auto [program, declarationCount] = parser.parse();
	    
	    std::cout << "====================================================" << std::endl;
	    std::cout << "Parsing completed successfully." << std::endl;
	    std::cout << "Total declarations: " << declarationCount << std::endl;
	    std::cout << "====================================================" << std::endl;
	} catch (const ParseError& e) {
	    std::cerr << "Parsing error: " << e.what() << std::endl;
	    return 1;
	}
    
    return 0;
}
