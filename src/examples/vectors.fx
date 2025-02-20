// Dynamic array implementation with capacity management
struct{T} Vector {
    T[] data;           // Raw array
    int{32} length;     // Current number of elements
    int{32} capacity;   // Total space allocated
};

// Create a new vector with initial capacity
Vector{T} create_vector{T}(int{32} initial_capacity) {
    Vector{T} vec;
    vec.data = new T[initial_capacity];
    vec.length = 0;
    vec.capacity = initial_capacity;
    return vec;
};

// Double the capacity of the vector
void grow{T}(Vector{T} *vec) {
    int{32} new_capacity = vec->capacity * 2;
    T[] new_data = new T[new_capacity];
    
    // Copy old elements to new array
    for (int{32} i = 0; i < vec->length; i = i + 1) {
        new_data[i] = vec->data[i];
    };
    
    // Free old array and update vector
    delete vec->data;
    vec->data = new_data;
    vec->capacity = new_capacity;
};

// Add element to end of vector
void push{T}(Vector{T} *vec, T element) {
    // Check if we need to grow
    if (vec->length == vec->capacity) {
        grow(vec);
    };
    
    vec->data[vec->length] = element;
    vec->length = vec->length + 1;
};

// Remove and return last element
T pop{T}(Vector{T} *vec) {
    if (vec->length == 0) {
        panic("Cannot pop from empty vector");
    };
    
    vec->length = vec->length - 1;
    return vec->data[vec->length];
};

// Get element at index
T get{T}(Vector{T} *vec, int{32} index) {
    if (index < 0 || index >= vec->length) {
        panic("Index out of bounds");
    };
    return vec->data[index];
};

// Set element at index
void set{T}(Vector{T} *vec, int{32} index, T value) {
    if (index < 0 || index >= vec->length) {
        panic("Index out of bounds");
    };
    vec->data[index] = value;
};

// Example usage with string processing
void reverse_words(string text) {
    Vector{string} words = create_vector(8);
    string current_word = "";
    
    // Split into words
    for (int{32} i = 0; i < length(text); i = i + 1) {
        if (text[i] == ' ') {
            if (length(current_word) > 0) {
                push(&words, current_word);
                current_word = "";
            };
        } else {
            current_word = current_word + text[i];
        };
    };
    
    // Add last word if exists
    if (length(current_word) > 0) {
        push(&words, current_word);
    };
    
    // Print words in reverse order
    for (int{32} i = words.length - 1; i >= 0; i = i - 1) {
        print(get(&words, i));
        if (i > 0) {
            print(" ");
        };
    };
    
    // Clean up
    delete words.data;
};

int{32} main(char[] *argc, char[][] *argv) {
    // Test vector with integers
    Vector{int} numbers = create_vector(4);
    
    // Add some numbers
    for (int{32} i = 0; i < 10; i = i + 1) {
        push(&numbers, i * i);
    };
    
    // Print squares
    print("First 10 square numbers:");
    for (int{32} i = 0; i < numbers.length; i = i + 1) {
        print(get(&numbers, i));
    };
    
    // Test string reversal
    print("\nReversed words:");
    reverse_words("Hello from the Flux language");
    
    // Clean up
    delete numbers.data;
    
    return 0;
};
