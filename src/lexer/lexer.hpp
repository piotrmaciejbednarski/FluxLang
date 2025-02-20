/**
 * @file lexer.hpp
 * @brief Lexer for Flux programming language
 */

#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <stdexcept>
#include "token.hpp"

namespace flux {

/**
 * @brief Lexer exception
 */
class LexerError : public std::runtime_error {
public:
    LexerError(const std::string& message, int line)
        : std::runtime_error(message), line(line) {}
    
    int getLine() const { return line; }
    
private:
    int line;
};

/**
 * @brief Lexer for Flux programming language
 * 
 * The Lexer converts source code into a sequence of tokens.
 */
class Lexer {
public:
    /**
     * @brief Construct a new Lexer object
     * 
     * @param source Source code to lex
     */
    explicit Lexer(std::string source);
    
    /**
     * @brief Tokenize the source code
     * 
     * @return std::vector<Token> Vector of tokens
     */
    std::vector<Token> scanTokens();
    
private:
    /**
     * @brief Scan a single token
     */
    void scanToken();
    
    /**
     * @brief Check if we've reached the end of the source
     * 
     * @return true if we've reached the end, false otherwise
     */
    bool isAtEnd() const;
    
    /**
     * @brief Advance to the next character
     * 
     * @return char The current character
     */
    char advance();
    
    /**
     * @brief Peek at the current character without consuming it
     * 
     * @return char The current character
     */
    char peek() const;
    
    /**
     * @brief Peek at the next character without consuming it
     * 
     * @return char The next character
     */
    char peekNext() const;
    
    /**
     * @brief Check if the current character matches the expected character
     * 
     * @param expected The expected character
     * @return true if the current character matches, false otherwise
     */
    bool match(char expected);
    
    /**
     * @brief Add a token with no literal value
     * 
     * @param type The token type
     */
    void addToken(TokenType type);
    
    /**
     * @brief Scan a string literal
     */
    void string();
    
    /**
     * @brief Scan an interpolated string
     */
    void interpolatedString();
    
    /**
     * @brief Scan a character literal
     */
    void character();
    
    /**
     * @brief Scan a number (integer or float)
     */
    void number();
    
    /**
     * @brief Scan an identifier or keyword
     */
    void identifier();
    
    /**
     * @brief Process an escape sequence in a string or character literal
     * 
     * @return char The processed escape character
     */
    char processEscapeSequence();
    
    /**
     * @brief Check if a character is a digit
     * 
     * @param c The character to check
     * @return true if the character is a digit, false otherwise
     */
    static bool isDigit(char c);
    
    /**
     * @brief Check if a character is a letter or underscore
     * 
     * @param c The character to check
     * @return true if the character is a letter or underscore, false otherwise
     */
    static bool isAlpha(char c);
    
    /**
     * @brief Check if a character is alphanumeric or underscore
     * 
     * @param c The character to check
     * @return true if the character is alphanumeric or underscore, false otherwise
     */
    static bool isAlphaNumeric(char c);
    
    /**
     * @brief Process a potential hexadecimal, octal, or binary number
     * 
     * @param prefix The prefix character ('x', 'b', etc.)
     */
    void processSpecialNumber(char prefix);
    
    /**
     * @brief Report an error
     * 
     * @param message The error message
     * @throws LexerError
     */
    void error(const std::string& message);
    
    // Member variables
    std::string source;      ///< Source code
    std::vector<Token> tokens; ///< Generated tokens
    int start = 0;           ///< Start of current lexeme
    int current = 0;         ///< Current position in source
    int line = 1;            ///< Current line number
    
    // Keyword map
    static const std::unordered_map<std::string, TokenType> keywords;
};

} // namespace flux
