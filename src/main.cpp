/**
 * @file main.cpp
 * @brief Simple test driver for the Flux language lexer and parser
 */

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <memory>
#include "lexer/lexer.hpp"
#include "parser/parser.hpp"
#include "forwards.hpp"
#include "parser/statements.hpp"
#include "parser/expressions.hpp"

using namespace flux;

/**
 * @brief Read the contents of a file into a string
 * 
 * @param filename Path to the file to read
 * @return std::string Contents of the file
 */
std::string readFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file: " + filename);
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <filename>" << std::endl;
        return 1;
    }
    
    std::string filename = argv[1];
    
    try {
        // Read the file
        std::string source = readFile(filename);
        
        std::cout << "Processing file: " << filename << std::endl;
        
        // Lexical analysis
        Lexer lexer(source);
        std::vector<Token> tokens = lexer.scanTokens();
        
        std::cout << "== Tokens ==" << std::endl;
        for (const auto& token : tokens) {
            if (token.type != TokenType::END_OF_FILE) {
                std::cout << "Type: " << static_cast<int>(token.type) 
                          << ", Lexeme: '" << token.lexeme 
                          << "', Line: " << token.line << std::endl;
            }
        }
        std::cout << "Total tokens: " << tokens.size() - 1 << std::endl; // Exclude EOF
        std::cout << std::endl;
        
        // Parsing
        Parser parser(tokens);
        std::unique_ptr<Program> program = parser.parse();
        
        std::cout << "== Abstract Syntax Tree ==" << std::endl;
        std::cout << "Successfully created AST with " << program->statements.size() << " top-level statements." << std::endl;
        
        // Print statement types
        for (size_t i = 0; i < program->statements.size(); i++) {
            std::cout << "Statement " << (i+1) << ": ";
            
            if (auto funcDecl = std::dynamic_pointer_cast<FunctionDeclarationStmt>(program->statements[i]))
                std::cout << "Function Declaration";
            else if (auto classDecl = std::dynamic_pointer_cast<ClassDeclarationStmt>(program->statements[i]))
                std::cout << "Class Declaration";
            else if (auto objDecl = std::dynamic_pointer_cast<ObjectDeclarationStmt>(program->statements[i]))
                std::cout << "Object Declaration";
            else if (auto nsDecl = std::dynamic_pointer_cast<NamespaceDeclarationStmt>(program->statements[i]))
                std::cout << "Namespace Declaration";
            else if (auto varDecl = std::dynamic_pointer_cast<VarDeclarationStmt>(program->statements[i]))
                std::cout << "Variable Declaration";
            else if (auto exprStmt = std::dynamic_pointer_cast<ExpressionStmt>(program->statements[i]))
                std::cout << "Expression Statement";
            else if (auto blockStmt = std::dynamic_pointer_cast<BlockStmt>(program->statements[i]))
                std::cout << "Block Statement";
            else if (auto ifStmt = std::dynamic_pointer_cast<IfStmt>(program->statements[i]))
                std::cout << "If Statement";
            else if (auto whileStmt = std::dynamic_pointer_cast<WhileStmt>(program->statements[i]))
                std::cout << "While Statement";
            else if (auto forStmt = std::dynamic_pointer_cast<ForStmt>(program->statements[i]))
                std::cout << "For Statement";
            else if (auto whenStmt = std::dynamic_pointer_cast<WhenStmt>(program->statements[i]))
                std::cout << "When Statement";
            else if (auto returnStmt = std::dynamic_pointer_cast<ReturnStmt>(program->statements[i]))
                std::cout << "Return Statement";
            else if (auto opDecl = std::dynamic_pointer_cast<OperatorDeclarationStmt>(program->statements[i]))
                std::cout << "Operator Declaration";
            else if (auto structDecl = std::dynamic_pointer_cast<StructDeclarationStmt>(program->statements[i]))
                std::cout << "Struct Declaration";
            else if (auto asmStmt = std::dynamic_pointer_cast<AsmStmt>(program->statements[i]))
                std::cout << "Assembly Statement";
            else
                std::cout << "Other Statement Type";
            
            std::cout << std::endl;
        }
        
        std::cout << "\nSuccessfully lexed and parsed " << filename << std::endl;
        
    } catch (const LexerError& e) {
        std::cerr << "Lexer error: " << e.what() << " (line " << e.getLine() << ")" << std::endl;
        return 1;
    } catch (const ParseError& e) {
        std::cerr << "Parser error: " << e.what() << std::endl;
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
