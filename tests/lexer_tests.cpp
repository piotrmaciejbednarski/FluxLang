/**
 * @file lexer_tests.cpp
 * @brief Test suite for the Flux lexer
 */

#include <iostream>
#include <string>
#include <vector>
#include <cassert>
#include "../src/lexer/lexer.hpp"
#include "../src/lexer/token.hpp"

using namespace flux;

// Helper function to test token type
void testTokenType(const Token& token, TokenType expectedType, const std::string& message) {
    if (token.type != expectedType) {
        std::cerr << "FAILED: " << message << "\n";
        std::cerr << "  Expected: " << static_cast<int>(expectedType) 
                  << ", Got: " << static_cast<int>(token.type) << "\n";
        assert(false);
    }
}

// Helper function to test token lexeme
void testTokenLexeme(const Token& token, const std::string& expectedLexeme, const std::string& message) {
    if (token.lexeme != expectedLexeme) {
        std::cerr << "FAILED: " << message << "\n";
        std::cerr << "  Expected: '" << expectedLexeme 
                  << "', Got: '" << token.lexeme << "'\n";
        assert(false);
    }
}

// Test simple token scanning
void testSimpleTokens() {
    std::cout << "Running simple tokens test...\n";
    
    std::string source = "( ) { } [ ] , . - + ; / * @ ~ ! = < > & | ^";
    Lexer lexer(source);
    std::vector<Token> tokens = lexer.scanTokens();
    
    // Last token should always be EOF
    assert(tokens.size() == 18);
    
    testTokenType(tokens[0], TokenType::LEFT_PAREN, "Left parenthesis token type");
    testTokenType(tokens[1], TokenType::RIGHT_PAREN, "Right parenthesis token type");
    testTokenType(tokens[2], TokenType::LEFT_BRACE, "Left brace token type");
    testTokenType(tokens[3], TokenType::RIGHT_BRACE, "Right brace token type");
    testTokenType(tokens[4], TokenType::LEFT_BRACKET, "Left bracket token type");
    testTokenType(tokens[5], TokenType::RIGHT_BRACKET, "Right bracket token type");
    testTokenType(tokens[6], TokenType::COMMA, "Comma token type");
    testTokenType(tokens[7], TokenType::DOT, "Dot token type");
    testTokenType(tokens[8], TokenType::MINUS, "Minus token type");
    testTokenType(tokens[9], TokenType::PLUS, "Plus token type");
    testTokenType(tokens[10], TokenType::SEMICOLON, "Semicolon token type");
    testTokenType(tokens[11], TokenType::SLASH, "Slash token type");
    testTokenType(tokens[12], TokenType::STAR, "Star token type");
    testTokenType(tokens[13], TokenType::AT, "At token type");
    testTokenType(tokens[14], TokenType::TILDE, "Tilde token type");
    testTokenType(tokens[15], TokenType::BANG, "Bang token type");
    testTokenType(tokens[16], TokenType::EQUAL, "Equal token type");
    
    std::cout << "Simple tokens test passed.\n";
}

// Test compound tokens
void testCompoundTokens() {
    std::cout << "Running compound tokens test...\n";
    
    std::string source = "!= == <= >= -> :: += -= *= /= %= &= |= ^= << >>";
    Lexer lexer(source);
    std::vector<Token> tokens = lexer.scanTokens();
    
    // Last token should always be EOF
    assert(tokens.size() == 16);
    
    testTokenType(tokens[0], TokenType::BANG_EQUAL, "Bang equal token type");
    testTokenType(tokens[1], TokenType::EQUAL_EQUAL, "Equal equal token type");
    testTokenType(tokens[2], TokenType::LESS_EQUAL, "Less equal token type");
    testTokenType(tokens[3], TokenType::GREATER_EQUAL, "Greater equal token type");
    testTokenType(tokens[4], TokenType::ARROW, "Arrow token type");
    testTokenType(tokens[5], TokenType::SCOPE_RESOLUTION, "Scope resolution token type");
    testTokenType(tokens[6], TokenType::PLUS_EQUAL, "Plus equal token type");
    testTokenType(tokens[7], TokenType::MINUS_EQUAL, "Minus equal token type");
    testTokenType(tokens[8], TokenType::STAR_EQUAL, "Star equal token type");
    testTokenType(tokens[9], TokenType::SLASH_EQUAL, "Slash equal token type");
    testTokenType(tokens[10], TokenType::PERCENT_EQUAL, "Percent equal token type");
    testTokenType(tokens[11], TokenType::BITAND_EQUAL, "Bitand equal token type");
    testTokenType(tokens[12], TokenType::BITOR_EQUAL, "Bitor equal token type");
    testTokenType(tokens[13], TokenType::XOR_EQUAL, "Xor equal token type");
    testTokenType(tokens[14], TokenType::SHIFT_LEFT, "Shift left token type");
    
    std::cout << "Compound tokens test passed.\n";
}

// Test literals
void testLiterals() {
    std::cout << "Running literals test...\n";
    
    std::string source = R"(
        "Hello, world!"
        'a'
        '\n'
        123
        123.456
        0x1A
        0b1010
        true
        false
        nullptr
    )";
    
    Lexer lexer(source);
    std::vector<Token> tokens = lexer.scanTokens();
    
    // Find the tokens by their lexemes
    int index = 0;
    while (index < tokens.size()) {
        if (tokens[index].lexeme == "\"Hello, world!\"") {
            testTokenType(tokens[index], TokenType::STRING, "String token type");
            break;
        }
        index++;
    }
    
    index = 0;
    while (index < tokens.size()) {
        if (tokens[index].lexeme == "'a'") {
            testTokenType(tokens[index], TokenType::CHAR, "Char token type");
            break;
        }
        index++;
    }
    
    index = 0;
    while (index < tokens.size()) {
        if (tokens[index].lexeme == "'\\n'") {
            testTokenType(tokens[index], TokenType::CHAR, "Escape char token type");
            break;
        }
        index++;
    }
    
    index = 0;
    while (index < tokens.size()) {
        if (tokens[index].lexeme == "123") {
            testTokenType(tokens[index], TokenType::INTEGER, "Integer token type");
            break;
        }
        index++;
    }
    
    index = 0;
    while (index < tokens.size()) {
        if (tokens[index].lexeme == "123.456") {
            testTokenType(tokens[index], TokenType::FLOAT, "Float token type");
            break;
        }
        index++;
    }
    
    index = 0;
    while (index < tokens.size()) {
        if (tokens[index].lexeme == "0x1A") {
            testTokenType(tokens[index], TokenType::INTEGER, "Hex integer token type");
            break;
        }
        index++;
    }
    
    index = 0;
    while (index < tokens.size()) {
        if (tokens[index].lexeme == "0b1010") {
            testTokenType(tokens[index], TokenType::INTEGER, "Binary integer token type");
            break;
        }
        index++;
    }
    
    index = 0;
    while (index < tokens.size()) {
        if (tokens[index].lexeme == "true") {
            testTokenType(tokens[index], TokenType::TRUE, "True token type");
            break;
        }
        index++;
    }
    
    index = 0;
    while (index < tokens.size()) {
        if (tokens[index].lexeme == "false") {
            testTokenType(tokens[index], TokenType::FALSE, "False token type");
            break;
        }
        index++;
    }
    
    index = 0;
    while (index < tokens.size()) {
        if (tokens[index].lexeme == "nullptr") {
            testTokenType(tokens[index], TokenType::NULL_LITERAL, "Nullptr token type");
            break;
        }
        index++;
    }
    
    std::cout << "Literals test passed.\n";
}

// Test keywords
void testKeywords() {
    std::cout << "Running keywords test...\n";
    
    std::string source = "object when asm async await int float char void bool volatile class";
    Lexer lexer(source);
    std::vector<Token> tokens = lexer.scanTokens();
    
    testTokenType(tokens[0], TokenType::OBJECT, "Object keyword");
    testTokenType(tokens[1], TokenType::WHEN, "When keyword");
    testTokenType(tokens[2], TokenType::ASM, "Asm keyword");
    testTokenType(tokens[3], TokenType::ASYNC, "Async keyword");
    testTokenType(tokens[4], TokenType::AWAIT, "Await keyword");
    testTokenType(tokens[5], TokenType::INT, "Int keyword");
    testTokenType(tokens[6], TokenType::FLOAT_TYPE, "Float keyword");
    testTokenType(tokens[7], TokenType::CHAR_TYPE, "Char keyword");
    testTokenType(tokens[8], TokenType::VOID, "Void keyword");
    testTokenType(tokens[9], TokenType::BOOL, "Bool keyword");
    testTokenType(tokens[10], TokenType::VOLATILE, "Volatile keyword");
    testTokenType(tokens[11], TokenType::CLASS, "Class keyword");
    
    std::cout << "Keywords test passed.\n";
}

// Test identifiers
void testIdentifiers() {
    std::cout << "Running identifiers test...\n";
    
    std::string source = "myVar _privateVar var123 camelCase snake_case";
    Lexer lexer(source);
    std::vector<Token> tokens = lexer.scanTokens();
    
    // All should be identifiers
    for (int i = 0; i < 5; i++) {
        testTokenType(tokens[i], TokenType::IDENTIFIER, "Identifier token type");
    }
    
    testTokenLexeme(tokens[0], "myVar", "First identifier");
    testTokenLexeme(tokens[1], "_privateVar", "Second identifier");
    testTokenLexeme(tokens[2], "var123", "Third identifier");
    testTokenLexeme(tokens[3], "camelCase", "Fourth identifier");
    testTokenLexeme(tokens[4], "snake_case", "Fifth identifier");
    
    std::cout << "Identifiers test passed.\n";
}

// Test comments
void testComments() {
    std::cout << "Running comments test...\n";
    
    std::string source = R"(
        // Single line comment
        int x = 5; // End of line comment
        /* Multi-line
           comment */
        int y = 10;
    )";
    
    Lexer lexer(source);
    std::vector<Token> tokens = lexer.scanTokens();
    
    // Comments should be ignored, so we should just have the tokens for the code
    // int x = 5; int y = 10;
    // 2 ints, 2 identifiers, 2 equals, 2 integers, 2 semicolons, and EOF
    // 9 tokens total
    assert(tokens.size() == 9);
    
    testTokenType(tokens[0], TokenType::INT, "Int after single line comment");
    testTokenType(tokens[1], TokenType::IDENTIFIER, "Identifier after single line comment");
    testTokenType(tokens[2], TokenType::EQUAL, "Equal after single line comment");
    testTokenType(tokens[3], TokenType::INTEGER, "Integer after single line comment");
    testTokenType(tokens[4], TokenType::SEMICOLON, "Semicolon after single line comment");
    testTokenType(tokens[5], TokenType::INT, "Int after multi-line comment");
    testTokenType(tokens[6], TokenType::IDENTIFIER, "Identifier after multi-line comment");
    testTokenType(tokens[7], TokenType::EQUAL, "Equal after multi-line comment");
    
    std::cout << "Comments test passed.\n";
}

// Test a simple Flux program
void testSimpleProgram() {
    std::cout << "Running simple program test...\n";
    
    std::string source = R"(
        int{32} fibonacci(int{32} n) {
            if (n <= 1) {
                return n;
            };
            
            return fibonacci(n - 1) + fibonacci(n - 2);
        };
        
        int{32} main() volatile {
            print(fibonacci(10));
            return 0;
        };
    )";
    
    Lexer lexer(source);
    std::vector<Token> tokens = lexer.scanTokens();
    
    // Just check that we have the right number of tokens and that lexing doesn't throw
    assert(tokens.size() > 10);
    assert(tokens.back().type == TokenType::END_OF_FILE);
    
    std::cout << "Simple program test passed.\n";
}

// Test error handling
void testErrorHandling() {
    std::cout << "Running error handling test...\n";
    
    // Unterminated string
    std::string source = R"("Unterminated string)";
    Lexer lexer(source);
    
    try {
        std::vector<Token> tokens = lexer.scanTokens();
        std::cerr << "FAILED: Expected lexer error for unterminated string\n";
        assert(false);
    } catch (const LexerError& e) {
        // Expected
    }
    
    // Unterminated character
    source = R"('a)";
    lexer = Lexer(source);
    
    try {
        std::vector<Token> tokens = lexer.scanTokens();
        std::cerr << "FAILED: Expected lexer error for unterminated character\n";
        assert(false);
    } catch (const LexerError& e) {
        // Expected
    }
    
    // Invalid escape sequence
    source = R"("\z")";
    lexer = Lexer(source);
    
    try {
        std::vector<Token> tokens = lexer.scanTokens();
        // We should still get a token, but with a warning
        // This is implementation dependent, so we don't assert here
    } catch (const LexerError& e) {
        // Also acceptable
    }
    
    std::cout << "Error handling test passed.\n";
}

// Test interpolated strings
void testInterpolatedStrings() {
    std::cout << "Running interpolated strings test...\n";
    
    std::string source = R"(i"{x} {y}":{x;y;})";
    Lexer lexer(source);
    std::vector<Token> tokens = lexer.scanTokens();
    
    // We should have i"{x} {y}", :, {, x, ;, y, ;, }, and EOF
    assert(tokens.size() == 9);
    
    testTokenType(tokens[0], TokenType::INTERPOLATED_STRING_START, "Interpolated string start");
    testTokenType(tokens[1], TokenType::COLON, "Colon after interpolated string");
    testTokenType(tokens[2], TokenType::LEFT_BRACE, "Left brace for interpolation values");
    testTokenType(tokens[3], TokenType::IDENTIFIER, "First interpolation identifier");
    testTokenType(tokens[4], TokenType::SEMICOLON, "Semicolon after first interpolation value");
    testTokenType(tokens[5], TokenType::IDENTIFIER, "Second interpolation identifier");
    testTokenType(tokens[6], TokenType::SEMICOLON, "Semicolon after second interpolation value");
    testTokenType(tokens[7], TokenType::RIGHT_BRACE, "Right brace for interpolation values");
    
    std::cout << "Interpolated strings test passed.\n";
}

int main() {
    testSimpleTokens();
    testCompoundTokens();
    testLiterals();
    testKeywords();
    testIdentifiers();
    testComments();
    testSimpleProgram();
    testErrorHandling();
    testInterpolatedStrings();
    
    std::cout << "All lexer tests passed!\n";
    return 0;
}
