// Memory-Efficient Flux Math Standard Library

typedef unsigned int{32} uint32;
typedef unsigned int{16} uint16;
typedef unsigned int{8} uint8;
typedef int{64} int64;
typedef int{32} int32;
typedef float{32} float32;
typedef float{64} float64;

namespace Math {    
    class MemoryArena {
        struct {
            byte[] buffer;
            uint32 capacity;
            uint32 used;
        } Arena;
        
            object ArenaOps {
                Arena create(uint32 size) {
                    Arena arena;
                    arena.capacity = size;
                    arena.used = 0;
                    arena.buffer = memalloc(byte[size]);
                    return arena;
                };
                
                void* allocate(Arena* arena, uint32 size) {
                    if (arena->used + size > arena->capacity) {
                        throw("Arena out of memory");
                    };
                    
                    void* ptr = &arena->buffer[arena->used];
                    arena->used += size;
                    return ptr;
                };
                
                void reset(Arena* arena) {
                    arena->used = 0;
                };
            };
    };
    
    //MemoryArena::Arena globalArena = MemoryArena::ArenaOps::create(65536);
    
    class Vector {
        struct {
            union {
                struct { float32 x, y, z, w; };
                float32[4] fixed;
                float32* dynamic;
            };
            uint16 size;
            bool usesDynamic;
        } Vector;

            object VectorOps {
                Vector create(uint16 size) {
                    Vector v;
                    v.size = size;
                    
                    if (size <= 4) {
                        v.usesDynamic = false;
                        for (uint16 i = 0; i < 4; i++) {
                            v.fixed[i] = 0.0;
                        };
                    } else {
                        v.usesDynamic = true;
                        v.dynamic = (float32*)MemoryArena::ArenaOps::allocate(
                            &globalArena, size * sizeof(float32)
                        );
                        
                        for (uint16 i = 0; i < size; i++) {
                            v.dynamic[i] = 0.0;
                        };
                    };
                    
                    return v;
                };
                
                float32 get(Vector* v, uint16 index) {
                    if (index >= v->size) {
                        throw("Vector index out of bounds");
                    };
                    
                    return v->usesDynamic ? v->dynamic[index] : v->fixed[index];
                };
                
                void set(Vector* v, uint16 index, float32 value) {
                    if (index >= v->size) {
                        throw("Vector index out of bounds");
                    };
                    
                    if (v->usesDynamic) {
                        v->dynamic[index] = value;
                    } else {
                        v->fixed[index] = value;
                    };
                };
                
                Vector fromArray(float32* arr, uint16 size) {
                    Vector v = create(size);
                    
                    for (uint16 i = 0; i < size; i++) {
                        set(&v, i, arr[i]);
                    };
                    
                    return v;
                };
                
                void add(Vector* result, Vector* a, Vector* b) {
                    if (a->size != b->size) {
                        throw("Vector dimensions don't match for addition");
                    };
                    
                    if (result->size != a->size) {
                        *result = create(a->size);
                    };
                    
                    for (uint16 i = 0; i < a->size; i++) {
                        set(result, i, get(a, i) + get(b, i));
                    };
                };
                
                void subtract(Vector* result, Vector* a, Vector* b) {
                    if (a->size != b->size) {
                        throw("Vector dimensions don't match for subtraction");
                    };
                    
                    if (result->size != a->size) {
                        *result = create(a->size);
                    };
                    
                    for (uint16 i = 0; i < a->size; i++) {
                        set(result, i, get(a, i) - get(b, i));
                    };
                };
                
                void scale(Vector* result, Vector* v, float32 scalar) {
                    if (result->size != v->size) {
                        *result = create(v->size);
                    };
                    
                    for (uint16 i = 0; i < v->size; i++) {
                        set(result, i, get(v, i) * scalar);
                    };
                };
                
                float32 dot(Vector* a, Vector* b) {
                    if (a->size != b->size) {
                        throw("Vector dimensions don't match for dot product");
                    };
                    
                    float32 result = 0.0;
                    
                    for (uint16 i = 0; i < a->size; i++) {
                        result += get(a, i) * get(b, i);
                    };
                    
                    return result;
                };
                
                void cross(Vector* result, Vector* a, Vector* b) {
                    if (a->size != 3 or b->size != 3) {
                        throw("Cross product is only defined for 3D vectors");
                    };
                    
                    if (result->size != 3) {
                        *result = create(3);
                    };
                    
                    float32 ax = get(a, 0);
                    float32 ay = get(a, 1);
                    float32 az = get(a, 2);
                    
                    float32 bx = get(b, 0);
                    float32 by = get(b, 1);
                    float32 bz = get(b, 2);
                    
                    set(result, 0, ay * bz - az * by);
                    set(result, 1, az * bx - ax * bz);
                    set(result, 2, ax * by - ay * bx);
                };
                
                float32 magnitude(Vector* v) {
                    float32 sum = 0.0;
                    
                    for (uint16 i = 0; i < v->size; i++) {
                        float32 component = get(v, i);
                        sum += component * component;
                    };
                    
                    return sqrt(sum);
                };
                
                void normalize(Vector* result, Vector* v) {
                    float32 mag = magnitude(v);
                    
                    if (mag < 1e-10) {
                        throw("Cannot normalize zero vector");
                    };
                    
                    scale(result, v, 1.0 / mag);
                };
                
                void print(Vector* v) {
                    print("[");
                    for (uint16 i = 0; i < v->size; i++) {
                        if (i > 0) print(", ");
                        print(get(v, i));
                    };
                    print("]");
                };
            };
    };
    
    class Matrix {
        struct {
            union {
                float32[16] fixed;
                float32* elements;
            };
            uint16 rows;
            uint16 cols;
            bool usesDynamic;
        } Matrix;
        
            object MatrixOps {
                float32 get(Matrix* m, uint16 row, uint16 col) {
                    if (row >= m->rows || col >= m->cols) {
                        throw("Matrix index out of bounds");
                    };
                    
                    uint32 index = row * m->cols + col;
                    return m->usesDynamic ? m->elements[index] : m->fixed[index];
                };
                
                void set(Matrix* m, uint16 row, uint16 col, float32 value) {
                    if (row >= m->rows || col >= m->cols) {
                        throw("Matrix index out of bounds");
                    };
                    
                    uint32 index = row * m->cols + col;
                    if (m->usesDynamic) {
                        m->elements[index] = value;
                    } else {
                        m->fixed[index] = value;
                    };
                };
                
                Matrix create(uint16 rows, uint16 cols) {
                    Matrix m;
                    m.rows = rows;
                    m.cols = cols;
                    
                    uint32 total = rows * cols;
                    
                    if (total <= 16) {
                        m.usesDynamic = false;
                        for (uint16 i = 0; i < 16; i++) {
                            m.fixed[i] = 0.0;
                        };
                    } else {
                        m.usesDynamic = true;
                        m.elements = (float32*)MemoryArena::ArenaOps::allocate(
                            &globalArena, total * sizeof(float32)
                        );
                        
                        for (uint32 i = 0; i < total; i++) {
                            m.elements[i] = 0.0;
                        };
                    };
                    
                    return m;
                };
                
                Matrix identity(uint16 size) {
                    Matrix m = create(size, size);
                    
                    for (uint16 i = 0; i < size; i++) {
                        set(&m, i, i, 1.0);
                    };
                    
                    return m;
                };
                
                void add(Matrix* result, Matrix* a, Matrix* b) {
                    if (a->rows != b->rows || a->cols != b->cols) {
                        throw("Matrix dimensions don't match for addition");
                    };
                    
                    if (result->rows != a->rows || result->cols != a->cols) {
                        *result = create(a->rows, a->cols);
                    };
                    
                    for (uint16 i = 0; i < a->rows; i++) {
                        for (uint16 j = 0; j < a->cols; j++) {
                            set(result, i, j, get(a, i, j) + get(b, i, j));
                        };
                    };
                };
                
                void subtract(Matrix* result, Matrix* a, Matrix* b) {
                    if (a->rows != b->rows || a->cols != b->cols) {
                        throw("Matrix dimensions don't match for subtraction");
                    };
                    
                    if (result->rows != a->rows || result->cols != a->cols) {
                        *result = create(a->rows, a->cols);
                    };
                    
                    for (uint16 i = 0; i < a->rows; i++) {
                        for (uint16 j = 0; j < a->cols; j++) {
                            set(result, i, j, get(a, i, j) - get(b, i, j));
                        };
                    };
                };
                
                void multiply(Matrix* result, Matrix* a, Matrix* b) {
                    if (a->cols != b->rows) {
                        throw("Matrix dimensions don't match for multiplication");
                    };
                    
                    if (result->rows != a->rows || result->cols != b->cols) {
                        *result = create(a->rows, b->cols);
                    };
                    
                    // Zero out result
                    for (uint16 i = 0; i < result->rows; i++) {
                        for (uint16 j = 0; j < result->cols; j++) {
                            set(result, i, j, 0.0);
                        };
                    };
                    
                    // Cache-friendly blocked matrix multiplication
                    const uint16 BLOCK_SIZE = 4;
                    
                    for (uint16 i0 = 0; i0 < a->rows; i0 += BLOCK_SIZE) {
                        for (uint16 j0 = 0; j0 < b->cols; j0 += BLOCK_SIZE) {
                            for (uint16 k0 = 0; k0 < a->cols; k0 += BLOCK_SIZE) {
                                uint16 iLimit = (i0 + BLOCK_SIZE < a->rows) ? i0 + BLOCK_SIZE : a->rows;
                                uint16 jLimit = (j0 + BLOCK_SIZE < b->cols) ? j0 + BLOCK_SIZE : b->cols;
                                uint16 kLimit = (k0 + BLOCK_SIZE < a->cols) ? k0 + BLOCK_SIZE : a->cols;
                                
                                for (uint16 i = i0; i < iLimit; i++) {
                                    for (uint16 j = j0; j < jLimit; j++) {
                                        float32 sum = get(result, i, j);
                                        
                                        for (uint16 k = k0; k < kLimit; k++) {
                                            sum += get(a, i, k) * get(b, k, j);
                                        };
                                        
                                        set(result, i, j, sum);
                                    };
                                };
                            };
                        };
                    };
                };
                
                void scale(Matrix* result, Matrix* m, float32 scalar) {
                    if (result->rows != m->rows || result->cols != m->cols) {
                        *result = create(m->rows, m->cols);
                    };
                    
                    for (uint16 i = 0; i < m->rows; i++) {
                        for (uint16 j = 0; j < m->cols; j++) {
                            set(result, i, j, get(m, i, j) * scalar);
                        };
                    };
                };
                
                void transpose(Matrix* result, Matrix* m) {
                    if (result == m) {
                        if (m->rows != m->cols) {
                            throw("In-place transpose requires square matrix");
                        };
                        
                        for (uint16 i = 0; i < m->rows; i++) {
                            for (uint16 j = i + 1; j < m->cols; j++) {
                                float32 temp = get(m, i, j);
                                set(m, i, j, get(m, j, i));
                                set(m, j, i, temp);
                            };
                        };
                    } else {
                        if (result->rows != m->cols || result->cols != m->rows) {
                            *result = create(m->cols, m->rows);
                        };
                        
                        for (uint16 i = 0; i < m->rows; i++) {
                            for (uint16 j = 0; j < m->cols; j++) {
                                set(result, j, i, get(m, i, j));
                            };
                        };
                    };
                };
                
                float32 determinant2x2(Matrix* m) {
                    return get(m, 0, 0) * get(m, 1, 1) - get(m, 0, 1) * get(m, 1, 0);
                };
                
                float32 determinant3x3(Matrix* m) {
                    float32 a = get(m, 0, 0);
                    float32 b = get(m, 0, 1);
                    float32 c = get(m, 0, 2);
                    float32 d = get(m, 1, 0);
                    float32 e = get(m, 1, 1);
                    float32 f = get(m, 1, 2);
                    float32 g = get(m, 2, 0);
                    float32 h = get(m, 2, 1);
                    float32 i = get(m, 2, 2);
                    
                    return a * (e * i - f * h) - b * (d * i - f * g) + c * (d * h - e * g);
                };
                
                float32 determinant(Matrix* m) {
                    if (m->rows != m->cols) {
                        throw("Determinant is only defined for square matrices");
                    };
                    
                    if (m->rows == 1) return get(m, 0, 0);
                    if (m->rows == 2) return determinant2x2(m);
                    if (m->rows == 3) return determinant3x3(m);
                    
                    // For larger matrices, use LU decomposition
                    Matrix lu = create(m->rows, m->cols);
                    for (uint16 i = 0; i < m->rows; i++) {
                        for (uint16 j = 0; j < m->cols; j++) {
                            set(&lu, i, j, get(m, i, j));
                        };
                    };
                    
                    int8* pivots = (int8*)MemoryArena::ArenaOps::allocate(
                        &globalArena, m->rows * sizeof(int8)
                    );
                    
                    int8 exchanges = 0;
                    
                    // LU decomposition with partial pivoting
                    for (uint16 i = 0; i < m->rows - 1; i++) {
                        float32 max_val = abs(get(&lu, i, i));
                        uint16 pivot_row = i;
                        
                        for (uint16 j = i + 1; j < m->rows; j++) {
                            float32 val = abs(get(&lu, j, i));
                            if (val > max_val) {
                                max_val = val;
                                pivot_row = j;
                            };
                        };
                        
                        if (max_val < 1e-10) return 0.0; // Singular
                        
                        if (pivot_row != i) {
                            exchanges++;
                            for (uint16 j = 0; j < m->cols; j++) {
                                float32 temp = get(&lu, i, j);
                                set(&lu, i, j, get(&lu, pivot_row, j));
                                set(&lu, pivot_row, j, temp);
                            };
                        };
                        
                        pivots[i] = pivot_row;
                        
                        for (uint16 j = i + 1; j < m->rows; j++) {
                            float32 factor = get(&lu, j, i) / get(&lu, i, i);
                            set(&lu, j, i, factor);
                            
                            for (uint16 k = i + 1; k < m->cols; k++) {
                                float32 val = get(&lu, j, k) - factor * get(&lu, i, k);
                                set(&lu, j, k, val);
                            };
                        };
                    };
                    
                    float32 det = (exchanges % 2 == 0) ? 1.0 : -1.0;
                    for (uint16 i = 0; i < m->rows; i++) {
                        det *= get(&lu, i, i);
                    };
                    
                    return det;
                };
                
                void print(Matrix* m) {
                    for (uint16 i = 0; i < m->rows; i++) {
                        print("[");
                        for (uint16 j = 0; j < m->cols; j++) {
                            if (j > 0) print(", ");
                            print(get(m, i, j));
                        };
                        print("]\n");
                    };
                };
            };
    };
    
    class Complex {
        struct {
            float32 real;
            float32 imag;
        } Complex;
        
            object ComplexOps {
                Complex create(float32 real, float32 imag) {
                    return {real, imag};
                };
                
                void add(Complex* result, Complex* a, Complex* b) {
                    result->real = a->real + b->real;
                    result->imag = a->imag + b->imag;
                };
                
                void subtract(Complex* result, Complex* a, Complex* b) {
                    result->real = a->real - b->real;
                    result->imag = a->imag - b->imag;
                };
                
                void multiply(Complex* result, Complex* a, Complex* b) {
                    float32 real = a->real * b->real - a->imag * b->imag;
                    float32 imag = a->real * b->imag + a->imag * b->real;
                    result->real = real;
                    result->imag = imag;
                };
                
                void divide(Complex* result, Complex* a, Complex* b) {
                    float32 denominator = b->real * b->real + b->imag * b->imag;
                    
                    if (denominator < 1e-10) {
                        throw("Division by zero");
                    };
                    
                    result->real = (a->real * b->real + a->imag * b->imag) / denominator;
                    result->imag = (a->imag * b->real - a->real * b->imag) / denominator;
                };
                
                float32 magnitude(Complex* c) {
                    return sqrt(c->real * c->real + c->imag * c->imag);
                };
                
                float32 phase(Complex* c) {
                    return atan2(c->imag, c->real);
                };
                
                void exp(Complex* result, Complex* c) {
                    float32 e_re = exp(c->real);
                    result->real = e_re * cos(c->imag);
                    result->imag = e_re * sin(c->imag);
                };
                
                void print(Complex* c) {
                    print(c->real);
                    if (c->imag >= 0) {
                        print(" + ");
                    } else {
                        print(" - ");
                    };
                    print(abs(c->imag));
                    print("i");
                };
            };
    };
};
