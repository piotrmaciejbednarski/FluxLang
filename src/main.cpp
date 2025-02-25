#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <memory>

#include "include/lexer.h"
#include "include/parser.h"
#include "include/interpreter.h"
#include "include/error.h"

// Helper function to read entire file contents
std::string readFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Could not open file: " << filename << std::endl;
        return "";
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

// Custom output buffer to capture stdout
class OutputCapture {
private:
    std::streambuf* oldCoutBuf;
    std::stringstream capturedOutput;
    bool isCapturing;

public:
    static bool disableCapture;
    OutputCapture() : oldCoutBuf(nullptr), isCapturing(false) {}

    void start() {
	    if (!isCapturing && !disableCapture) {
		oldCoutBuf = std::cout.rdbuf();
		std::cout.rdbuf(capturedOutput.rdbuf());
		isCapturing = true;
	    }
	}

    void stop() {
        if (isCapturing) {
            std::cout.rdbuf(oldCoutBuf);
            isCapturing = false;
        }
    }

    std::string get() {
        return capturedOutput.str();
    }

    void clear() {
        capturedOutput.str("");
        capturedOutput.clear();
    }
};

bool OutputCapture::disableCapture = false;

int main(int argc, char* argv[]) {
    // Check if filename is provided
    if (argc < 2) {
        std::cerr << "Usage: flux_compiler <filename.fx>" << std::endl;
        return 1;
    }

    // Read source file
    std::string sourceCode = readFile(argv[1]);
    if (sourceCode.empty()) {
        return 1;
    }

    // Initialize lexer
    flux::Lexer lexer(sourceCode);
    
    // Tokenize the source code
    std::vector<flux::Token> tokens = lexer.scanTokens();

    // Print tokens (optional, for debugging)
    std::cout << "Tokens:" << std::endl;
    for (const auto& token : tokens) {
        std::cout << "Type: " << static_cast<int>(token.type) 
                  << ", Lexeme: " << token.lexeme 
                  << ", Line: " << token.line 
                  << ", Column: " << token.column << std::endl;
    }

    // Initialize parser
    flux::Parser parser;
    
    std::cout << "Source code:" << std::endl;
    for (int i = 0; i < sourceCode.length(); i++) {
        char c = sourceCode[i];
        if (c == '\n') {
            std::cout << "\\n" << std::endl;
        } else if (c == '\t') {
            std::cout << "\\t";
        } else if (c < 32 || c > 126) {
            std::cout << "\\x" << std::hex << (int)c;
        } else {
            std::cout << c;
        }
    }
    std::cout << std::endl;

    std::cout << "Tokens: " << tokens.size() << std::endl;
    for (size_t i = 0; i < std::min(tokens.size(), size_t(20)); i++) {
        const auto& token = tokens[i];
        std::cout << "Token " << i << ": Type=" << static_cast<int>(token.type) 
                  << ", Lexeme='" << token.lexeme 
                  << "', Line=" << token.line 
                  << ", Column=" << token.column << std::endl;
    }
    
    // Parse tokens into AST
    std::shared_ptr<flux::Program> program = nullptr;
    try {
        program = parser.parse(tokens, argv[1]);
    } catch (const std::exception& e) {
        std::cerr << "Parsing error: " << e.what() << std::endl;
        return 1;
    }

    // Check for parsing errors
    if (parser.hasError()) {
        std::cerr << "Parsing completed with errors." << std::endl;
        
        // Retrieve and print errors
        auto errors = flux::errorReporter.getErrors();
        for (const auto& error : errors) {
            error.report();
        }
        
        return 1;
    }

    std::cout << "Parsing completed successfully!" << std::endl;

    // Set up output capturing
    OutputCapture outputCapture;
    outputCapture.start();

    // Initialize interpreter and run program
    flux::Interpreter interpreter;
    interpreter.initialize();
    
    try {
        interpreter.interpret(program);
    } catch (const flux::RuntimeError& e) {
        outputCapture.stop();
        std::cerr << "Runtime error: " << e.what() << std::endl;
        return 1;
    }

        outputCapture.stop();
    
    // Output the result with the [Result] header
    std::string result = outputCapture.get();
    std::cout << "\n[Result]" << std::endl;
    std::cout << result;
    
    // Debug to see if capture is working
    if (result.empty()) {
        std::cout << "No output captured. Bypassing capture mechanism:" << std::endl;
        
        // Try executing without capturing
        OutputCapture::disableCapture = true;  // Add this static flag to OutputCapture class
        interpreter.interpret(program);
    }

    return 0;
}
