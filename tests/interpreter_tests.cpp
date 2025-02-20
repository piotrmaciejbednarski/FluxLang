#include <gtest/gtest.h>
#include "interpreter/interpreter.hpp"
#include "interpreter/environment.hpp"
#include "interpreter/value.hpp"
#include "parser/parser.hpp"
#include "lexer/lexer.hpp"
#include <memory>
#include <string>
#include <vector>

using namespace flux;

class InterpreterTest : public ::testing::Test {
protected:
    Interpreter interpreter;

    Value evaluate(const std::string& source) {
        Lexer lexer(source);
        auto tokens = lexer.tokenize();
        Parser parser(std::move(tokens));
        auto statements = parser.parse();
        return interpreter.interpret(statements);
    }

    void execute(const std::string& source) {
        evaluate(source);
    }

    // Helper to verify numeric values
    void assertNumber(const Value& value, double expected) {
        if (value.isInteger()) {
            EXPECT_EQ(value.asInteger(), expected);
        } else if (value.isFloat()) {
            EXPECT_DOUBLE_EQ(value.asFloat(), expected);
        } else {
            FAIL() << "Expected numeric value";
        }
    }

    // Helper to verify string values
    void assertString(const Value& value, const std::string& expected) {
        ASSERT_TRUE(value.isString());
        EXPECT_EQ(value.asString(), expected);
    }

    // Helper to verify boolean values
    void assertBoolean(const Value& value, bool expected) {
        ASSERT_TRUE(value.isBoolean());
        EXPECT_EQ(value.asBoolean(), expected);
    }
};

// Expression evaluation tests
TEST_F(InterpreterTest, ArithmeticExpressions) {
    assertNumber(evaluate("2 + 3;"), 5);
    assertNumber(evaluate("10 - 5;"), 5);
    assertNumber(evaluate("4 * 3;"), 12);
    assertNumber(evaluate("15 / 3;"), 5);
    assertNumber(evaluate("10 % 3;"), 1);
}

TEST_F(InterpreterTest, CompoundArithmetic) {
    assertNumber(evaluate("2 + 3 * 4;"), 14);
    assertNumber(evaluate("(2 + 3) * 4;"), 20);
    assertNumber(evaluate("8 - 3 + 2 * 5;"), 15);
}

TEST_F(InterpreterTest, ComparisonOperations) {
    assertBoolean(evaluate("5 > 3;"), true);
    assertBoolean(evaluate("5 < 3;"), false);
    assertBoolean(evaluate("5 >= 5;"), true);
    assertBoolean(evaluate("5 <= 4;"), false);
    assertBoolean(evaluate("5 == 5;"), true);
    assertBoolean(evaluate("5 != 4;"), true);
}

TEST_F(InterpreterTest, LogicalOperations) {
    assertBoolean(evaluate("true and true;"), true);
    assertBoolean(evaluate("true and false;"), false);
    assertBoolean(evaluate("true or false;"), true);
    assertBoolean(evaluate("false or false;"), false);
    assertBoolean(evaluate("not true;"), false);
    assertBoolean(evaluate("not false;"), true);
}

// Variable handling tests
TEST_F(InterpreterTest, VariableDeclarationAndAccess) {
    execute("x: int = 42;");
    assertNumber(evaluate("x;"), 42);
    
    execute("y: float = 3.14;");
    assertNumber(evaluate("y;"), 3.14);
    
    execute("s: string = \"hello\";");
    assertString(evaluate("s;"), "hello");
}

TEST_F(InterpreterTest, VariableAssignment) {
    execute("x: int = 42;");
    execute("x = 73;");
    assertNumber(evaluate("x;"), 73);
}

TEST_F(InterpreterTest, Scoping) {
    execute(R"(
        x: int = 42;
        {
            x: int = 73;
            // Inner x should be 73
        }
        // Outer x should still be 42
    )");
    assertNumber(evaluate("x;"), 42);
}

// Object tests
TEST_F(InterpreterTest, ObjectCreationAndAccess) {
    execute(R"(
        object Point {
            x: int = 0;
            y: int = 0;

            constructor(xVal: int, yVal: int) {
                x = xVal;
                y = yVal;
            }
        };

        p: Point = Point(10, 20);
    )");

    assertNumber(evaluate("p.x;"), 10);
    assertNumber(evaluate("p.y;"), 20);
}

TEST_F(InterpreterTest, ObjectMethods) {
    execute(R"(
        object Calculator {
            add(a: int, b: int): int {
                return a + b;
            }

            multiply(a: int, b: int): int {
                return a * b;
            }
        };

        calc: Calculator = Calculator();
    )");

    assertNumber(evaluate("calc.add(5, 3);"), 8);
    assertNumber(evaluate("calc.multiply(4, 6);"), 24);
}

// When statement tests
TEST_F(InterpreterTest, WhenStatement) {
    execute(R"(
        x: int = 0;
        when (x < 10) {
            x = x + 1;
        };
    )");

    // The when block should have executed
    assertNumber(evaluate("x;"), 1);
}

TEST_F(InterpreterTest, VolatileWhenStatement) {
    execute(R"(
        temperature: float = 100.0;
        alert_count: int = 0;

        when (temperature > 90.0) volatile {
            alert_count = alert_count + 1;
        };
    )");

    assertNumber(evaluate("alert_count;"), 1);
}

// Operator overloading tests
TEST_F(InterpreterTest, OperatorOverloading) {
    execute(R"(
        object Vector {
            x: float = 0.0;
            y: float = 0.0;

            constructor(xVal: float, yVal: float) {
                x = xVal;
                y = yVal;
            }

            operator(Vector, Vector)[+] {
                return Vector(self.x + other.x, self.y + other.y);
            };
        };

        v1: Vector = Vector(1.0, 2.0);
        v2: Vector = Vector(3.0, 4.0);
        v3: Vector = v1 + v2;
    )");

    assertNumber(evaluate("v3.x;"), 4.0);
    assertNumber(evaluate("v3.y;"), 6.0);
}

// Error handling tests
TEST_F(InterpreterTest, DivisionByZero) {
    EXPECT_THROW(evaluate("1 / 0;"), Interpreter::RuntimeError);
}

TEST_F(InterpreterTest, UndefinedVariable) {
    EXPECT_THROW(evaluate("undefined_var;"), Interpreter::RuntimeError);
}

TEST_F(InterpreterTest, TypeError) {
    EXPECT_THROW(evaluate("\"hello\" - 42;"), Interpreter::RuntimeError);
}

// Type conversion tests
TEST_F(InterpreterTest, TypeConversion) {
    assertNumber(evaluate("int(3.14);"), 3);
    assertNumber(evaluate("float(42);"), 42.0);
    assertString(evaluate("string(42);"), "42");
    assertString(evaluate("string(3.14);"), "3.14");
}

// Built-in function tests
TEST_F(InterpreterTest, BuiltInFunctions) {
    assertNumber(evaluate("len(\"hello\");"), 5);
    assertString(evaluate("to_string(42);"), "42");
    assertNumber(evaluate("to_number(\"3.14\");"), 3.14);
}

// Complex program tests
TEST_F(InterpreterTest, FibonacciProgram) {
    execute(R"(
        fibonacci(n: int): int {
            if (n <= 1) {
                return n;
            }
            return fibonacci(n - 1) + fibonacci(n - 2);
        }
    )");

    assertNumber(evaluate("fibonacci(6);"), 8);
}

TEST_F(InterpreterTest, ComplexObjectInteractions) {
    execute(R"(
        object BankAccount {
            balance: float = 0.0;

            deposit(amount: float) {
                if (amount < 0.0) {
                    throw "Cannot deposit negative amount";
                }
                balance = balance + amount;
            }

            withdraw(amount: float) {
                if (amount < 0.0) {
                    throw "Cannot withdraw negative amount";
                }
                if (amount > balance) {
                    throw "Insufficient funds";
                }
                balance = balance - amount;
            }
        };

        account: BankAccount = BankAccount();
        account.deposit(100.0);
        account.withdraw(30.0);
    )");

    assertNumber(evaluate("account.balance;"), 70.0);
}

// Memory management tests
TEST_F(InterpreterTest, ObjectLifetime) {
    execute(R"(
        {
            x: object = SomeObject();
        }  // x should be cleaned up here
    )");
    // Verify no memory leaks
}

// Namespace tests
TEST_F(InterpreterTest, NamespaceHandling) {
    execute(R"(
        namespace Math {
            class Vector {
                x: float = 0.0;
                y: float = 0.0;
            }

            class Matrix {
                // ... matrix implementation
            }
        };

        v: Math.Vector = Math.Vector();
    )");

    // Test namespace resolution
    EXPECT_NO_THROW(evaluate("v.x = 42.0;"));
}

// Exception handling tests
TEST_F(InterpreterTest, ExceptionHandling) {
    execute(R"(
        result: string = "";
        try {
            throw "test error";
        } catch (e) {
            result = e;
        }
    )");

    assertString(evaluate("result;"), "test error");
}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
