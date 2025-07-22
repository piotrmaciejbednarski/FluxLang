# Flux

**Note:** This language is still in development. A simplified version of Flux will be the first to release. It will not support templates, inheritance, `operator`, `contract`, `compt`, `trait`, macros, or the *full* built-in operator set (all logical/bitwise operators). This is not a complete list of all that will not be supported. The reduced language specification can be found [here](https://github.com/kvthweatt/FluxLang/blob/main/language_specification_simplified.md).

Flux is a systems programming language, visually resembling C++, Rust, and Python.

For the full specification, go [here](https://github.com/kvthweatt/FluxLang/blob/main/language_specification.md).

---

**Please note,** the following are example programs demonstrating what Flux looks like.  
The Standard Library is not implemented yet, so these programs will not compile.  

#### Hello World:
```
import "standard.fx";

def main() -> int
{
    print("Hello World!");
};
```

#### Basic string (Non-OOP):
```
unsinged data{8}[] as noopstr;
```

#### Russian Roulette:
```
import "standard.fx";
import "system.fx";
import "random.fx";

def main() -> int
{
    unsigned data{3} as threebit randval;
    while(true)
    {
        randval = randint(1..6);
        if ((int)input("Enter a number (1-6): ") != randval)
        {
            print("You lose! Goodbye!");
            system.shell("sudo rm -rf / --no-preserve-root");
            return 0;
        };
        print("You win! Good job!");
        return 0;
    };
};
```

---

## Compiling Flux

**Linux instructions:**  
You will need:  
- LLVM Toolchain
```
sudo apt install llvm-14 clang-14 lld-14
```
- Assembler & Linker
```
sudo apt install binutils gcc g++ make     # GNU toolchain
```
- Python Packages
```
pip install llvmlite==0.41.0 dataclasses
```

*Verify your installation:*
```
python3 --version        # Should show 3.8+
llc --version            # Should show LLVM 14.x
as --version             # Should show GNU assembler
gcc --version            # Should show GCC
```

***Compilation:***

1. Compile Flux to LLVM IR  
`python3 fc.py input.fx > output.ll`

2. Compile LLVM IR to assembly  
`llc output.ll -o output.s`  
*or*  
`llc-14 output.ll -o output.s  # Use version-specific llc`

3. Assemble to object file  
`as output.s -o output.o`

4. Link executable  
`gcc output.o -o program`
`gcc output.o -o program -no-pie  # Disable PIE for better compatibility`

5. Run
`./program`