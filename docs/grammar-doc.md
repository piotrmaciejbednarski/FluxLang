# Flux Language Grammar

## Lexical Grammar

### Keywords
```ebnf
KEYWORD ::= 'object' | 'when' | 'asm' | 'and' | 'and_eq' | 'assert' | 'async' 
          | 'await' | 'or' | 'bitand' | 'bitor' | 'break' | 'case' | 'catch' 
          | 'char' | 'class' | 'const' | 'continue' | 'default' | 'delete' 
          | 'do' | 'double' | 'else' | 'enum' | 'false' | 'float' | 'for' 
          | 'goto' | 'if' | 'int' | 'is' | 'long' | 'namespace' | 'new' 
          | 'not' | 'not_eq' | 'nullptr' | 'operator' | 'or_eq' | 'requires' 
          | 'return' | 'short' | 'signed' | 'sizeof' | 'struct' | 'switch' 
          | 'this' | 'throw' | 'true' | 'try' | 'typedef' | 'union' 
          | 'unsigned' | 'using' | 'void' | 'volatile' | 'while' | 'xor' 
          | 'xor_eq' | 'print' | 'input' | 'memalloc' | 'sizeof'
```

### Literals
```ebnf
INTEGER    ::= DECIMAL_INT | HEX_INT | OCTAL_INT | BINARY_INT
DECIMAL_INT ::= DIGIT+ [INT_SUFFIX]
HEX_INT    ::= '0' ('x'|'X') HEX_DIGIT+ [INT_SUFFIX]
OCTAL_INT  ::= '0' OCTAL_DIGIT+ [INT_SUFFIX]
BINARY_INT ::= '0' ('b'|'B') ('0'|'1')+ [INT_SUFFIX]
INT_SUFFIX ::= ('u'|'U') ['l'|'L'] ['l'|'L'] | ('l'|'L') ['l'|'L']

FLOAT      ::= DIGIT+ '.' DIGIT+ [EXPONENT] [FLOAT_SUFFIX]
             | DIGIT+ EXPONENT [FLOAT_SUFFIX]
EXPONENT   ::= ('e'|'E') ['+'|'-'] DIGIT+
FLOAT_SUFFIX ::= 'f' | 'F' | 'l' | 'L'

STRING     ::= '"' (CHAR | ESCAPE_SEQ)* '"'
CHAR_LIT   ::= "'" (CHAR | ESCAPE_SEQ) "'"
ESCAPE_SEQ ::= '\' ('n'|'r'|'t'|'\'|'"'|'0')
```

### Operators
```ebnf
OPERATOR ::= '+' | '-' | '*' | '/' | '%' | '^' | '&' | '|' | '~' | '!' | '=' 
           | '<' | '>' | '+=' | '-=' | '*=' | '/=' | '%=' | '^=' | '&=' | '|=' 
           | '<<' | '>>' | '<=' | '>=' | '==' | '!=' | '->' | '->*' | '.' 
           | '.*' | '...'
```

### Delimiters
```ebnf
DELIMITER ::= '(' | ')' | '{' | '}' | '[' | ']' | ';' | ':' | ','
```

### Identifiers
```ebnf
IDENTIFIER ::= (LETTER | '_') (LETTER | DIGIT | '_')*
LETTER     ::= [a-zA-Z]
DIGIT      ::= [0-9]
```

## Syntactic Grammar

### Program Structure
```ebnf
Program     ::= Declaration*
Declaration ::= ObjectDecl
              | ClassDecl
              | NamespaceDecl
              | OperatorDecl
              | Statement

ObjectDecl  ::= 'object' IDENTIFIER '{' Method* Member* '}' ';'
ClassDecl   ::= 'class' IDENTIFIER '{' Method* Member* '}' ';'
NamespaceDecl ::= 'namespace' IDENTIFIER '{' ClassDecl* '}' ';'
```

### Declarations and Definitions
```ebnf
Method      ::= Type IDENTIFIER '(' Parameters? ')' Block
Parameters  ::= Parameter (',' Parameter)*
Parameter   ::= Type IDENTIFIER

Member      ::= Type IDENTIFIER ['=' Expression] ';'

OperatorDecl ::= 'operator' '(' Type ',' Type ')' '[' Operator ']' 
                 '{' Statement* '}' ';'
```

### Statements
```ebnf
Statement   ::= ExprStmt
              | WhenStmt
              | BlockStmt
              | ReturnStmt
              | VarDecl

ExprStmt    ::= Expression ';'
WhenStmt    ::= 'when' '(' Expression ')' ['volatile'] Block ';'
BlockStmt   ::= '{' Statement* '}'
ReturnStmt  ::= 'return' Expression? ';'
VarDecl     ::= Type IDENTIFIER ['=' Expression] ';'
```

### Expressions
```ebnf
Expression  ::= Assignment
Assignment  ::= LogicalOr (AssignOp LogicalOr)*
LogicalOr   ::= LogicalAnd ('or' LogicalAnd)*
LogicalAnd  ::= Equality ('and' Equality)*
Equality    ::= Comparison (('=='|'!='|'is') Comparison)*
Comparison  ::= Term (('<'|'<='|'>'|'>=') Term)*
Term        ::= Factor (('+'|'-') Factor)*
Factor      ::= Unary (('*'|'/'|'%') Unary)*
Unary       ::= ('!'|'-'|'~') Unary | Call
Call        ::= Primary ('(' Arguments? ')')*
Primary     ::= IDENTIFIER | Literal | '(' Expression ')'
Arguments   ::= Expression (',' Expression)*
```

### Types
```ebnf
Type        ::= BasicType
              | ArrayType
              | PointerType
BasicType   ::= 'int' | 'float' | 'char' | 'void' | 'bool' | IDENTIFIER
ArrayType   ::= Type '[' Expression? ']'
PointerType ::= Type '*'
```

## Operator Precedence (highest to lowest)

1. Scope resolution `::`
2. Member access `.`, `->`, `.*`, `->*`
3. Function call `()`, array subscript `[]`
4. Unary `+`, `-`, `!`, `~`
5. Multiplication `*`, `/`, `%`
6. Addition `+`, `-`
7. Bitwise shift `<<`, `>>`
8. Relational `<`, `<=`, `>`, `>=`
9. Equality `==`, `!=`, `is`
10. Bitwise AND `&`
11. Bitwise XOR `^`
12. Bitwise OR `|`
13. Logical AND `and`
14. Logical OR `or`
15. Assignment `=`, `+=`, `-=`, `*=`, `/=`, `%=`, `&=`, `|=`, `^=`
