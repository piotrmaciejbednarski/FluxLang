"""
Flux Output Writer (foutput.py)

A comprehensive output formatting system for the Flux compiler toolchain.
Provides multiple output formats for tokens, AST nodes, and other compiler data.
"""

from typing import List, Optional, Any, Dict, Union
from enum import Enum, auto
import sys
from dataclasses import dataclass


class OutputFormat(Enum):
    """Available output formatting styles"""
    PLAIN = auto()          # Simple text output
    TABLE = auto()          # Formatted table
    COMPACT = auto()        # Compact single-line format
    DEBUG = auto()          # Detailed debug information
    COLORED = auto()        # Color-coded output
    JSON = auto()           # JSON format
    XML = auto()            # XML format


class ColorCode:
    """ANSI color codes for terminal output"""
    # Text colors
    BLACK = '\033[30m'
    RED = '\033[31m'
    GREEN = '\033[32m'
    YELLOW = '\033[33m'
    BLUE = '\033[34m'
    MAGENTA = '\033[35m'
    CYAN = '\033[36m'
    WHITE = '\033[37m'
    
    # Bright colors
    BRIGHT_BLACK = '\033[90m'
    BRIGHT_RED = '\033[91m'
    BRIGHT_GREEN = '\033[92m'
    BRIGHT_YELLOW = '\033[93m'
    BRIGHT_BLUE = '\033[94m'
    BRIGHT_MAGENTA = '\033[95m'
    BRIGHT_CYAN = '\033[96m'
    BRIGHT_WHITE = '\033[97m'
    
    # Background colors
    BG_BLACK = '\033[40m'
    BG_RED = '\033[41m'
    BG_GREEN = '\033[42m'
    BG_YELLOW = '\033[43m'
    BG_BLUE = '\033[44m'
    BG_MAGENTA = '\033[45m'
    BG_CYAN = '\033[46m'
    BG_WHITE = '\033[47m'
    
    # Styles
    BOLD = '\033[1m'
    DIM = '\033[2m'
    ITALIC = '\033[3m'
    UNDERLINE = '\033[4m'
    BLINK = '\033[5m'
    REVERSE = '\033[7m'
    STRIKETHROUGH = '\033[9m'
    
    # Reset
    RESET = '\033[0m'
    
    @staticmethod
    def supports_color() -> bool:
        """Check if the terminal supports color output"""
        return (hasattr(sys.stdout, 'isatty') and sys.stdout.isatty() and 
                sys.platform != 'win32')


@dataclass
class OutputConfig:
    """Configuration for output formatting"""
    format: OutputFormat = OutputFormat.PLAIN
    show_line_numbers: bool = True
    show_column_numbers: bool = True
    show_token_values: bool = True
    group_by_line: bool = False
    max_width: int = 80
    indent_size: int = 2
    use_colors: bool = None  # None = auto-detect
    filter_types: Optional[List[str]] = None  # Filter specific token types
    
    def __post_init__(self):
        if self.use_colors is None:
            self.use_colors = ColorCode.supports_color()


class FluxOutputWriter:
    """
    Flexible output writer for Flux compiler components.
    Supports multiple output formats and styling options.
    """
    
    def __init__(self, config: Optional[OutputConfig] = None):
        self.config = config or OutputConfig()
        
        # Token type color mapping
        self.token_colors = {
            'KEYWORD': ColorCode.BLUE + ColorCode.BOLD,
            'IDENTIFIER': ColorCode.WHITE,
            'INTEGER': ColorCode.CYAN,
            'FLOAT': ColorCode.CYAN,
            'STRING': ColorCode.GREEN,
            'CHAR': ColorCode.GREEN,
            'BOOLEAN': ColorCode.MAGENTA,
            'BINARY': ColorCode.CYAN,
            'COMMENT': ColorCode.BRIGHT_BLACK,
            'OPERATOR': ColorCode.YELLOW,
            'PUNCTUATION': ColorCode.WHITE,
            'DATA_TYPE': ColorCode.BLUE,
            'ASM_BLOCK': ColorCode.RED,
            'ISTRING_START': ColorCode.BRIGHT_GREEN,
            'EOF': ColorCode.DIM,
        }
    
    def colorize(self, text: str, color: str) -> str:
        """Apply color to text if colors are enabled"""
        if not self.config.use_colors:
            return text
        return f"{color}{text}{ColorCode.RESET}"
    
    def get_token_color(self, token_type: str) -> str:
        """Get color for a specific token type"""
        # Group token types into categories
        operators = ['PLUS', 'MINUS', 'MULTIPLY', 'DIVIDE', 'MODULO', 'POWER',
                    'ASSIGN', 'PLUS_ASSIGN', 'MINUS_ASSIGN', 'MULTIPLY_ASSIGN',
                    'EQUAL', 'NOT_EQUAL', 'LESS', 'GREATER', 'AND', 'OR', 'XOR',
                    'BITWISE_AND', 'BITWISE_OR', 'BITWISE_XOR', 'INCREMENT', 'DECREMENT']
        
        punctuation = ['SEMICOLON', 'COMMA', 'DOT', 'SCOPE', 'COLON', 'QUESTION',
                      'LPAREN', 'RPAREN', 'LBRACE', 'RBRACE', 'LBRACKET', 'RBRACKET']
        
        if token_type in self.token_colors:
            return self.token_colors[token_type]
        elif token_type in operators:
            return self.token_colors['OPERATOR']
        elif token_type in punctuation:
            return self.token_colors['PUNCTUATION']
        else:
            return ColorCode.WHITE
    
    def format_token_plain(self, token) -> str:
        """Format a single token in plain text"""
        pos = ""
        if self.config.show_line_numbers:
            pos += f"{token.line}"
        if self.config.show_column_numbers:
            pos += f":{token.column}" if pos else f"{token.column}"
        
        if pos:
            pos = f"[{pos}] "
        
        value_part = f" = '{token.value}'" if self.config.show_token_values and token.value else ""
        
        return f"{pos}{token.type.name}{value_part}"
    
    def format_token_colored(self, token) -> str:
        """Format a single token with colors"""
        pos = ""
        if self.config.show_line_numbers:
            pos += self.colorize(str(token.line), ColorCode.DIM)
        if self.config.show_column_numbers:
            pos += self.colorize(f":{token.column}", ColorCode.DIM) if pos else self.colorize(str(token.column), ColorCode.DIM)
        
        if pos:
            pos = f"[{pos}] "
        
        token_type = self.colorize(token.type.name, self.get_token_color(token.type.name))
        
        value_part = ""
        if self.config.show_token_values and token.value:
            color = self.get_token_color(token.type.name)
            value_part = f" = {self.colorize(repr(token.value), color)}"
        
        return f"{pos}{token_type}{value_part}"
    
    def format_token_compact(self, token) -> str:
        """Format a token in compact form"""
        if token.value:
            return f"{token.type.name}({repr(token.value)})"
        return token.type.name
    
    def format_tokens_table(self, tokens: List) -> str:
        """Format tokens as a table"""
        if not tokens:
            return "No tokens to display."
        
        # Calculate column widths
        max_line = max(len(str(token.line)) for token in tokens)
        max_col = max(len(str(token.column)) for token in tokens)
        max_type = max(len(token.type.name) for token in tokens)
        max_value = max(len(repr(token.value)) for token in tokens if token.value)
        max_value = min(max_value, 40)  # Limit value column width
        
        # Header
        header_parts = []
        if self.config.show_line_numbers:
            header_parts.append(f"{'Line':<{max_line}}")
        if self.config.show_column_numbers:
            header_parts.append(f"{'Col':<{max_col}}")
        header_parts.append(f"{'Type':<{max_type}}")
        if self.config.show_token_values:
            header_parts.append(f"{'Value':<{max_value}}")
        
        header = " | ".join(header_parts)
        separator = "-" * len(header)
        
        result = [header, separator]
        
        # Rows
        for token in tokens:
            if self.config.filter_types and token.type.name not in self.config.filter_types:
                continue
                
            row_parts = []
            if self.config.show_line_numbers:
                line_str = str(token.line)
                if self.config.use_colors:
                    line_str = self.colorize(line_str, ColorCode.DIM)
                row_parts.append(f"{line_str:<{max_line}}")
            
            if self.config.show_column_numbers:
                col_str = str(token.column)
                if self.config.use_colors:
                    col_str = self.colorize(col_str, ColorCode.DIM)
                row_parts.append(f"{col_str:<{max_col}}")
            
            type_str = token.type.name
            if self.config.use_colors:
                type_str = self.colorize(type_str, self.get_token_color(token.type.name))
            row_parts.append(f"{type_str:<{max_type}}")
            
            if self.config.show_token_values:
                value_str = repr(token.value) if token.value else ""
                if len(value_str) > max_value:
                    value_str = value_str[:max_value-3] + "..."
                if self.config.use_colors and token.value:
                    value_str = self.colorize(value_str, self.get_token_color(token.type.name))
                row_parts.append(f"{value_str:<{max_value}}")
            
            result.append(" | ".join(row_parts))
        
        return "\n".join(result)
    
    def format_tokens_grouped(self, tokens: List) -> str:
        """Format tokens grouped by line"""
        if not tokens:
            return "No tokens to display."
        
        # Group tokens by line
        lines = {}
        for token in tokens:
            if token.line not in lines:
                lines[token.line] = []
            lines[token.line].append(token)
        
        result = []
        for line_num in sorted(lines.keys()):
            line_tokens = lines[line_num]
            line_header = f"Line {line_num}:"
            if self.config.use_colors:
                line_header = self.colorize(line_header, ColorCode.BOLD + ColorCode.YELLOW)
            
            result.append(line_header)
            
            for token in line_tokens:
                if self.config.filter_types and token.type.name not in self.config.filter_types:
                    continue
                
                indent = " " * self.config.indent_size
                if self.config.format == OutputFormat.COLORED:
                    token_str = self.format_token_colored(token)
                else:
                    token_str = self.format_token_plain(token)
                
                result.append(f"{indent}{token_str}")
            
            result.append("")  # Empty line between groups
        
        return "\n".join(result)
    
    def format_tokens_json(self, tokens: List) -> str:
        """Format tokens as JSON"""
        import json
        
        token_dicts = []
        for token in tokens:
            if self.config.filter_types and token.type.name not in self.config.filter_types:
                continue
                
            token_dict = {
                "type": token.type.name,
                "value": token.value,
                "line": token.line,
                "column": token.column
            }
            token_dicts.append(token_dict)
        
        return json.dumps(token_dicts, indent=2)
    
    def format_tokens_xml(self, tokens: List) -> str:
        """Format tokens as XML"""
        result = ['<?xml version="1.0" encoding="UTF-8"?>', '<tokens>']
        
        for token in tokens:
            if self.config.filter_types and token.type.name not in self.config.filter_types:
                continue
            
            # Escape XML special characters
            value = token.value.replace('&', '&amp;').replace('<', '&lt;').replace('>', '&gt;').replace('"', '&quot;').replace("'", '&apos;')
            
            result.append(f'  <token type="{token.type.name}" line="{token.line}" column="{token.column}" value="{value}"/>')
        
        result.append('</tokens>')
        return '\n'.join(result)
    
    def write_tokens(self, tokens: List, title: Optional[str] = None) -> str:
        """Main method to format and return tokens according to configuration"""
        if not tokens:
            return "No tokens to display."
        
        # Filter tokens if specified
        if self.config.filter_types:
            tokens = [t for t in tokens if t.type.name in self.config.filter_types]
        
        output_parts = []
        
        # Add title if provided
        if title:
            if self.config.use_colors:
                title = self.colorize(title, ColorCode.BOLD + ColorCode.CYAN)
            output_parts.append(title)
            output_parts.append("=" * len(title.replace('\033[0m', '').replace('\033[1m\033[36m', '')))
        
        # Format according to selected format
        if self.config.format == OutputFormat.TABLE:
            output_parts.append(self.format_tokens_table(tokens))
        elif self.config.format == OutputFormat.COMPACT:
            compact_tokens = [self.format_token_compact(token) for token in tokens]
            # Group compact tokens by lines for readability
            lines = []
            current_line = []
            current_length = 0
            
            for token_str in compact_tokens:
                if current_length + len(token_str) + 2 > self.config.max_width:
                    if current_line:
                        lines.append(" ".join(current_line))
                        current_line = [token_str]
                        current_length = len(token_str)
                    else:
                        lines.append(token_str)
                        current_length = 0
                else:
                    current_line.append(token_str)
                    current_length += len(token_str) + 1
            
            if current_line:
                lines.append(" ".join(current_line))
            
            output_parts.append("\n".join(lines))
        elif self.config.format == OutputFormat.JSON:
            output_parts.append(self.format_tokens_json(tokens))
        elif self.config.format == OutputFormat.XML:
            output_parts.append(self.format_tokens_xml(tokens))
        elif self.config.group_by_line:
            output_parts.append(self.format_tokens_grouped(tokens))
        else:  # PLAIN, COLORED, DEBUG
            for token in tokens:
                if self.config.format == OutputFormat.COLORED:
                    output_parts.append(self.format_token_colored(token))
                elif self.config.format == OutputFormat.DEBUG:
                    output_parts.append(f"DEBUG: {token} (pos={getattr(token, 'pos', 'N/A')})")
                else:
                    output_parts.append(self.format_token_plain(token))
        
        return "\n".join(output_parts)
    
    def print_tokens(self, tokens: List, title: Optional[str] = None):
        """Print formatted tokens to stdout"""
        print(self.write_tokens(tokens, title))
    
    def write_to_file(self, tokens: List, filename: str, title: Optional[str] = None):
        """Write formatted tokens to a file"""
        with open(filename, 'w', encoding='utf-8') as f:
            # Disable colors for file output
            original_use_colors = self.config.use_colors
            self.config.use_colors = False
            
            try:
                f.write(self.write_tokens(tokens, title))
            finally:
                self.config.use_colors = original_use_colors


# Convenience functions for common use cases
def print_tokens_simple(tokens: List, title: Optional[str] = None):
    """Simple token printing with default formatting"""
    writer = FluxOutputWriter()
    writer.print_tokens(tokens, title)


def print_tokens_table(tokens: List, title: Optional[str] = None):
    """Print tokens in table format"""
    config = OutputConfig(format=OutputFormat.TABLE)
    writer = FluxOutputWriter(config)
    writer.print_tokens(tokens, title)


def print_tokens_compact(tokens: List, title: Optional[str] = None):
    """Print tokens in compact format"""
    config = OutputConfig(format=OutputFormat.COMPACT)
    writer = FluxOutputWriter(config)
    writer.print_tokens(tokens, title)


def print_tokens_colored(tokens: List, title: Optional[str] = None):
    """Print tokens with color coding"""
    config = OutputConfig(format=OutputFormat.COLORED)
    writer = FluxOutputWriter(config)
    writer.print_tokens(tokens, title)


def print_tokens_grouped(tokens: List, title: Optional[str] = None):
    """Print tokens grouped by line"""
    config = OutputConfig(format=OutputFormat.PLAIN, group_by_line=True)
    writer = FluxOutputWriter(config)
    writer.print_tokens(tokens, title)


# Example usage
if __name__ == "__main__":
    # Mock token class for testing
    from enum import Enum, auto
    from dataclasses import dataclass
    
    class TokenType(Enum):
        KEYWORD = auto()
        IDENTIFIER = auto()
        INTEGER = auto()
        STRING = auto()
        SEMICOLON = auto()
    
    @dataclass
    class Token:
        type: TokenType
        value: str
        line: int
        column: int
    
    # Sample tokens
    sample_tokens = [
        Token(TokenType.KEYWORD, "def", 1, 1),
        Token(TokenType.IDENTIFIER, "main", 1, 5),
        Token(TokenType.INTEGER, "42", 2, 10),
        Token(TokenType.STRING, "Hello", 2, 15),
        Token(TokenType.SEMICOLON, ";", 2, 22),
    ]
    
    print("=== Different Output Formats ===\n")
    
    print("1. Plain format:")
    print_tokens_simple(sample_tokens)
    
    print("\n2. Table format:")
    print_tokens_table(sample_tokens)
    
    print("\n3. Compact format:")
    print_tokens_compact(sample_tokens)
    
    print("\n4. Colored format:")
    print_tokens_colored(sample_tokens)
    
    print("\n5. Grouped format:")
    print_tokens_grouped(sample_tokens)