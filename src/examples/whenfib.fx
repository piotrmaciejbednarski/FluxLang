int{32} fibonacci(int{32} *n) {
    when (n == 10) {
        print(i"Reached Fibonacci index 10, value: {value}":{fibonacci(n);});
    };

    if (n <= 1) {
        return n;
    };
    
    return fibonacci(n - 1) + fibonacci(n - 2);
};

int{32} main() volatile {
    x = 5;
    int{32} *px = @x;
    fibonacci(*px);
    return 0;
};
