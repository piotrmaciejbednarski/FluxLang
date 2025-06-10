// ================================================================
// FLUX MATRICES LIBRARY
// matrix.fx - Comprehensive matrix operations and utilities
// ================================================================

import "types.fx";
import "math.fx";

namespace standard
{
    namespace matrix
    {
        using standard::types::basic;
        using standard::math;
        
        // ================================================================
        // BASIC MATRIX TYPES
        // ================================================================
        
        namespace basic
        {
            // Generic matrix object
            object matrix
            {
                float* data;
                basic::i32 rows;
                basic::i32 cols;
                basic::i32 capacity;
                
                def __init(basic::i32 r, basic::i32 c) -> this
                {
                    this.rows = r;
                    this.cols = c;
                    this.capacity = r * c;
                    // TODO: Allocate data array of size capacity
                    return this;
                };
                
                def __init() -> this
                {
                    this.rows = 0;
                    this.cols = 0;
                    this.capacity = 0;
                    this.data = void;
                    return this;
                };
                
                def __exit() -> void
                {
                    if (this.data is !void)
                    {
                        (void)this.data;
                    };
                    return void;
                };
                
                def get_rows() -> basic::i32
                {
                    return this.rows;
                };
                
                def get_cols() -> basic::i32
                {
                    return this.cols;
                };
                
                def get_size() -> basic::i32
                {
                    return this.rows * this.cols;
                };
                
                def is_square() -> bool
                {
                    return this.rows == this.cols;
                };
                
                def is_empty() -> bool
                {
                    return this.rows == 0 or this.cols == 0;
                };
                
                def at(basic::i32 row, basic::i32 col) -> float
                {
                    if (row >= this.rows or col >= this.cols or row < 0 or col < 0)
                    {
                        throw("Matrix index out of bounds");
                    };
                    return this.data[row * this.cols + col];
                };
                
                def set(basic::i32 row, basic::i32 col, float value) -> void
                {
                    if (row >= this.rows or col >= this.cols or row < 0 or col < 0)
                    {
                        throw("Matrix index out of bounds");
                    };
                    this.data[row * this.cols + col] = value;
                    return void;
                };
                
                def resize(basic::i32 new_rows, basic::i32 new_cols) -> void
                {
                    basic::i32 new_capacity = new_rows * new_cols;
                    if (new_capacity > this.capacity)
                    {
                        if (this.data is !void)
                        {
                            (void)this.data;
                        };
                        // TODO: Allocate new data array
                        this.capacity = new_capacity;
                    };
                    this.rows = new_rows;
                    this.cols = new_cols;
                    return void;
                };
                
                def fill(float value) -> void
                {
                    for (basic::i32 i = 0; i < this.get_size(); i++)
                    {
                        this.data[i] = value;
                    };
                    return void;
                };
                
                def zero() -> void
                {
                    this.fill(0.0);
                    return void;
                };
                
                def identity() -> void
                {
                    if (!this.is_square())
                    {
                        throw("Identity matrix must be square");
                    };
                    
                    this.zero();
                    for (basic::i32 i = 0; i < this.rows; i++)
                    {
                        this.set(i, i, 1.0);
                    };
                    return void;
                };
                
                def copy_from(matrix other) -> void
                {
                    this.resize(other.rows, other.cols);
                    for (basic::i32 i = 0; i < this.get_size(); i++)
                    {
                        this.data[i] = other.data[i];
                    };
                    return void;
                };
                
                def __eq(matrix other) -> void
                {
                    this.copy_from(other);
                    return void;
                };
                
                def __ee(matrix other) -> bool
                {
                    if (this.rows != other.rows or this.cols != other.cols)
                    {
                        return false;
                    };
                    
                    for (basic::i32 i = 0; i < this.get_size(); i++)
                    {
                        if (math::basic::abs(this.data[i] - other.data[i]) > math::constants::FLOAT_EPSILON)
                        {
                            return false;
                        };
                    };
                    
                    return true;
                };
                
                def __ne(matrix other) -> bool
                {
                    return !(this.__ee(other));
                };
                
                def __add(matrix other) -> matrix
                {
                    if (this.rows != other.rows or this.cols != other.cols)
                    {
                        throw("Matrix dimensions must match for addition");
                    };
                    
                    matrix result(this.rows, this.cols);
                    for (basic::i32 i = 0; i < this.get_size(); i++)
                    {
                        result.data[i] = this.data[i] + other.data[i];
                    };
                    
                    return result;
                };
                
                def __sub(matrix other) -> matrix
                {
                    if (this.rows != other.rows or this.cols != other.cols)
                    {
                        throw("Matrix dimensions must match for subtraction");
                    };
                    
                    matrix result(this.rows, this.cols);
                    for (basic::i32 i = 0; i < this.get_size(); i++)
                    {
                        result.data[i] = this.data[i] - other.data[i];
                    };
                    
                    return result;
                };
                
                def __mul(float scalar) -> matrix
                {
                    matrix result(this.rows, this.cols);
                    for (basic::i32 i = 0; i < this.get_size(); i++)
                    {
                        result.data[i] = this.data[i] * scalar;
                    };
                    
                    return result;
                };
                
                def __mul(matrix other) -> matrix
                {
                    if (this.cols != other.rows)
                    {
                        throw("Matrix dimensions incompatible for multiplication");
                    };
                    
                    matrix result(this.rows, other.cols);
                    result.zero();
                    
                    for (basic::i32 i = 0; i < this.rows; i++)
                    {
                        for (basic::i32 j = 0; j < other.cols; j++)
                        {
                            float sum = 0.0;
                            for (basic::i32 k = 0; k < this.cols; k++)
                            {
                                sum += this.at(i, k) * other.at(k, j);
                            };
                            result.set(i, j, sum);
                        };
                    };
                    
                    return result;
                };
                
                def __div(float scalar) -> matrix
                {
                    if (math::basic::abs(scalar) < math::constants::FLOAT_EPSILON)
                    {
                        throw("Division by zero in matrix scalar division");
                    };
                    
                    matrix result(this.rows, this.cols);
                    for (basic::i32 i = 0; i < this.get_size(); i++)
                    {
                        result.data[i] = this.data[i] / scalar;
                    };
                    
                    return result;
                };
                
                def transpose() -> matrix
                {
                    matrix result(this.cols, this.rows);
                    for (basic::i32 i = 0; i < this.rows; i++)
                    {
                        for (basic::i32 j = 0; j < this.cols; j++)
                        {
                            result.set(j, i, this.at(i, j));
                        };
                    };
                    
                    return result;
                };
                
                def trace() -> float
                {
                    if (!this.is_square())
                    {
                        throw("Trace is only defined for square matrices");
                    };
                    
                    float sum = 0.0;
                    for (basic::i32 i = 0; i < this.rows; i++)
                    {
                        sum += this.at(i, i);
                    };
                    
                    return sum;
                };
                
                def frobenius_norm() -> float
                {
                    float sum = 0.0;
                    for (basic::i32 i = 0; i < this.get_size(); i++)
                    {
                        sum += this.data[i] * this.data[i];
                    };
                    
                    return math::basic::sqrt(sum);
                };
                
                def max_norm() -> float
                {
                    float max_val = 0.0;
                    for (basic::i32 i = 0; i < this.get_size(); i++)
                    {
                        max_val = math::basic::max(max_val, math::basic::abs(this.data[i]));
                    };
                    
                    return max_val;
                };
                
                def get_row(basic::i32 row) -> matrix
                {
                    if (row >= this.rows or row < 0)
                    {
                        throw("Row index out of bounds");
                    };
                    
                    matrix result(1, this.cols);
                    for (basic::i32 j = 0; j < this.cols; j++)
                    {
                        result.set(0, j, this.at(row, j));
                    };
                    
                    return result;
                };
                
                def get_col(basic::i32 col) -> matrix
                {
                    if (col >= this.cols or col < 0)
                    {
                        throw("Column index out of bounds");
                    };
                    
                    matrix result(this.rows, 1);
                    for (basic::i32 i = 0; i < this.rows; i++)
                    {
                        result.set(i, 0, this.at(i, col));
                    };
                    
                    return result;
                };
                
                def set_row(basic::i32 row, matrix row_data) -> void
                {
                    if (row >= this.rows or row < 0)
                    {
                        throw("Row index out of bounds");
                    };
                    if (row_data.cols != this.cols or row_data.rows != 1)
                    {
                        throw("Row data dimensions incompatible");
                    };
                    
                    for (basic::i32 j = 0; j < this.cols; j++)
                    {
                        this.set(row, j, row_data.at(0, j));
                    };
                    return void;
                };
                
                def set_col(basic::i32 col, matrix col_data) -> void
                {
                    if (col >= this.cols or col < 0)
                    {
                        throw("Column index out of bounds");
                    };
                    if (col_data.rows != this.rows or col_data.cols != 1)
                    {
                        throw("Column data dimensions incompatible");
                    };
                    
                    for (basic::i32 i = 0; i < this.rows; i++)
                    {
                        this.set(i, col, col_data.at(i, 0));
                    };
                    return void;
                };
                
                def swap_rows(basic::i32 row1, basic::i32 row2) -> void
                {
                    if (row1 >= this.rows or row2 >= this.rows or row1 < 0 or row2 < 0)
                    {
                        throw("Row index out of bounds");
                    };
                    if (row1 == row2) { return void; };
                    
                    for (basic::i32 j = 0; j < this.cols; j++)
                    {
                        float temp = this.at(row1, j);
                        this.set(row1, j, this.at(row2, j));
                        this.set(row2, j, temp);
                    };
                    return void;
                };
                
                def swap_cols(basic::i32 col1, basic::i32 col2) -> void
                {
                    if (col1 >= this.cols or col2 >= this.cols or col1 < 0 or col2 < 0)
                    {
                        throw("Column index out of bounds");
                    };
                    if (col1 == col2) { return void; };
                    
                    for (basic::i32 i = 0; i < this.rows; i++)
                    {
                        float temp = this.at(i, col1);
                        this.set(i, col1, this.at(i, col2));
                        this.set(i, col2, temp);
                    };
                    return void;
                };
            };
            
            // Fixed-size matrices for common dimensions
            object matrix2x2
            {
                float data[4];
                
                def __init() -> this
                {
                    for (basic::i32 i = 0; i < 4; i++)
                    {
                        this.data[i] = 0.0;
                    };
                    return this;
                };
                
                def __init(float m00, float m01, float m10, float m11) -> this
                {
                    this.data[0] = m00; this.data[1] = m01;
                    this.data[2] = m10; this.data[3] = m11;
                    return this;
                };
                
                def __exit() -> void
                {
                    return void;
                };
                
                def at(basic::i32 row, basic::i32 col) -> float
                {
                    return this.data[row * 2 + col];
                };
                
                def set(basic::i32 row, basic::i32 col, float value) -> void
                {
                    this.data[row * 2 + col] = value;
                    return void;
                };
                
                def determinant() -> float
                {
                    return this.data[0] * this.data[3] - this.data[1] * this.data[2];
                };
                
                def inverse() -> matrix2x2
                {
                    float det = this.determinant();
                    if (math::basic::abs(det) < math::constants::FLOAT_EPSILON)
                    {
                        throw("Matrix is singular, cannot compute inverse");
                    };
                    
                    matrix2x2 result(this.data[3] / det, -this.data[1] / det, 
                                   -this.data[2] / det, this.data[0] / det);
                    return result;
                };
                
                def __mul(matrix2x2 other) -> matrix2x2
                {
                    matrix2x2 result();
                    result.data[0] = this.data[0] * other.data[0] + this.data[1] * other.data[2];
                    result.data[1] = this.data[0] * other.data[1] + this.data[1] * other.data[3];
                    result.data[2] = this.data[2] * other.data[0] + this.data[3] * other.data[2];
                    result.data[3] = this.data[2] * other.data[1] + this.data[3] * other.data[3];
                    return result;
                };
                
                def to_matrix() -> matrix
                {
                    matrix result(2, 2);
                    for (basic::i32 i = 0; i < 4; i++)
                    {
                        result.data[i] = this.data[i];
                    };
                    return result;
                };
            };
            
            object matrix3x3
            {
                float data[9];
                
                def __init() -> this
                {
                    for (basic::i32 i = 0; i < 9; i++)
                    {
                        this.data[i] = 0.0;
                    };
                    return this;
                };
                
                def __init(float m00, float m01, float m02,
                           float m10, float m11, float m12,
                           float m20, float m21, float m22) -> this
                {
                    this.data[0] = m00; this.data[1] = m01; this.data[2] = m02;
                    this.data[3] = m10; this.data[4] = m11; this.data[5] = m12;
                    this.data[6] = m20; this.data[7] = m21; this.data[8] = m22;
                    return this;
                };
                
                def __exit() -> void
                {
                    return void;
                };
                
                def at(basic::i32 row, basic::i32 col) -> float
                {
                    return this.data[row * 3 + col];
                };
                
                def set(basic::i32 row, basic::i32 col, float value) -> void
                {
                    this.data[row * 3 + col] = value;
                    return void;
                };
                
                def determinant() -> float
                {
                    return this.data[0] * (this.data[4] * this.data[8] - this.data[5] * this.data[7]) -
                           this.data[1] * (this.data[3] * this.data[8] - this.data[5] * this.data[6]) +
                           this.data[2] * (this.data[3] * this.data[7] - this.data[4] * this.data[6]);
                };
                
                def to_matrix() -> matrix
                {
                    matrix result(3, 3);
                    for (basic::i32 i = 0; i < 9; i++)
                    {
                        result.data[i] = this.data[i];
                    };
                    return result;
                };
            };
            
            object matrix4x4
            {
                float data[16];
                
                def __init() -> this
                {
                    for (basic::i32 i = 0; i < 16; i++)
                    {
                        this.data[i] = 0.0;
                    };
                    return this;
                };
                
                def __exit() -> void
                {
                    return void;
                };
                
                def at(basic::i32 row, basic::i32 col) -> float
                {
                    return this.data[row * 4 + col];
                };
                
                def set(basic::i32 row, basic::i32 col, float value) -> void
                {
                    this.data[row * 4 + col] = value;
                    return void;
                };
                
                def to_matrix() -> matrix
                {
                    matrix result(4, 4);
                    for (basic::i32 i = 0; i < 16; i++)
                    {
                        result.data[i] = this.data[i];
                    };
                    return result;
                };
            };
        };
        
        // ================================================================
        // MATRIX OPERATIONS
        // ================================================================
        
        namespace operations
        {
            // Matrix creation utilities
            def create_matrix(basic::i32 rows, basic::i32 cols, float initial_value) -> basic::matrix
            {
                basic::matrix result(rows, cols);
                result.fill(initial_value);
                return result;
            };
            
            def zeros(basic::i32 rows, basic::i32 cols) -> basic::matrix
            {
                return create_matrix(rows, cols, 0.0);
            };
            
            def ones(basic::i32 rows, basic::i32 cols) -> basic::matrix
            {
                return create_matrix(rows, cols, 1.0);
            };
            
            def identity(basic::i32 size) -> basic::matrix
            {
                basic::matrix result(size, size);
                result.identity();
                return result;
            };
            
            def diagonal(float values[], basic::i32 count) -> basic::matrix
            {
                basic::matrix result(count, count);
                result.zero();
                for (basic::i32 i = 0; i < count; i++)
                {
                    result.set(i, i, values[i]);
                };
                return result;
            };
            
            def random_matrix(basic::i32 rows, basic::i32 cols, float min_val, float max_val) -> basic::matrix
            {
                basic::matrix result(rows, cols);
                for (basic::i32 i = 0; i < result.get_size(); i++)
                {
                    result.data[i] = math::random::rand_range(min_val, max_val);
                };
                return result;
            };
            
            // Matrix arithmetic operations
            def add(basic::matrix a, basic::matrix b) -> basic::matrix
            {
                return a.__add(b);
            };
            
            def subtract(basic::matrix a, basic::matrix b) -> basic::matrix
            {
                return a.__sub(b);
            };
            
            def multiply(basic::matrix a, basic::matrix b) -> basic::matrix
            {
                return a.__mul(b);
            };
            
            def scalar_multiply(basic::matrix a, float scalar) -> basic::matrix
            {
                return a.__mul(scalar);
            };
            
            def element_wise_multiply(basic::matrix a, basic::matrix b) -> basic::matrix
            {
                if (a.rows != b.rows or a.cols != b.cols)
                {
                    throw("Matrix dimensions must match for element-wise multiplication");
                };
                
                basic::matrix result(a.rows, a.cols);
                for (basic::i32 i = 0; i < a.get_size(); i++)
                {
                    result.data[i] = a.data[i] * b.data[i];
                };
                
                return result;
            };
            
            def element_wise_divide(basic::matrix a, basic::matrix b) -> basic::matrix
            {
                if (a.rows != b.rows or a.cols != b.cols)
                {
                    throw("Matrix dimensions must match for element-wise division");
                };
                
                basic::matrix result(a.rows, a.cols);
                for (basic::i32 i = 0; i < a.get_size(); i++)
                {
                    if (math::basic::abs(b.data[i]) < math::constants::FLOAT_EPSILON)
                    {
                        throw("Division by zero in element-wise division");
                    };
                    result.data[i] = a.data[i] / b.data[i];
                };
                
                return result;
            };
            
            def power(basic::matrix a, basic::i32 exponent) -> basic::matrix
            {
                if (!a.is_square())
                {
                    throw("Matrix power is only defined for square matrices");
                };
                if (exponent < 0)
                {
                    throw("Negative exponents not supported");
                };
                if (exponent == 0)
                {
                    return identity(a.rows);
                };
                if (exponent == 1)
                {
                    basic::matrix result();
                    result.copy_from(a);
                    return result;
                };
                
                basic::matrix result = identity(a.rows);
                basic::matrix base();
                base.copy_from(a);
                
                while (exponent > 0)
                {
                    if (exponent % 2 == 1)
                    {
                        result = multiply(result, base);
                    };
                    base = multiply(base, base);
                    exponent /= 2;
                };
                
                return result;
            };
            
            // Matrix decomposition and solving
            def determinant(basic::matrix a) -> float
            {
                if (!a.is_square())
                {
                    throw("Determinant is only defined for square matrices");
                };
                
                basic::i32 n = a.rows;
                if (n == 1)
                {
                    return a.at(0, 0);
                };
                if (n == 2)
                {
                    return a.at(0, 0) * a.at(1, 1) - a.at(0, 1) * a.at(1, 0);
                };
                
                // Use LU decomposition for larger matrices
                basic::matrix lu();
                lu.copy_from(a);
                basic::i32 sign = 1;
                
                // Gaussian elimination with partial pivoting
                for (basic::i32 i = 0; i < n - 1; i++)
                {
                    // Find pivot
                    basic::i32 max_row = i;
                    for (basic::i32 k = i + 1; k < n; k++)
                    {
                        if (math::basic::abs(lu.at(k, i)) > math::basic::abs(lu.at(max_row, i)))
                        {
                            max_row = k;
                        };
                    };
                    
                    if (max_row != i)
                    {
                        lu.swap_rows(i, max_row);
                        sign = -sign;
                    };
                    
                    if (math::basic::abs(lu.at(i, i)) < math::constants::FLOAT_EPSILON)
                    {
                        return 0.0; // Singular matrix
                    };
                    
                    // Eliminate below pivot
                    for (basic::i32 k = i + 1; k < n; k++)
                    {
                        float factor = lu.at(k, i) / lu.at(i, i);
                        for (basic::i32 j = i + 1; j < n; j++)
                        {
                            lu.set(k, j, lu.at(k, j) - factor * lu.at(i, j));
                        };
                    };
                };
                
                // Calculate determinant as product of diagonal elements
                float det = (float)sign;
                for (basic::i32 i = 0; i < n; i++)
                {
                    det *= lu.at(i, i);
                };
                
                return det;
            };
            
            def inverse(basic::matrix a) -> basic::matrix
            {
                if (!a.is_square())
                {
                    throw("Inverse is only defined for square matrices");
                };
                
                basic::i32 n = a.rows;
                
                // Create augmented matrix [A|I]
                basic::matrix augmented(n, 2 * n);
                for (basic::i32 i = 0; i < n; i++)
                {
                    for (basic::i32 j = 0; j < n; j++)
                    {
                        augmented.set(i, j, a.at(i, j));
                        augmented.set(i, j + n, (i == j) ? 1.0 : 0.0);
                    };
                };
                
                // Gauss-Jordan elimination
                for (basic::i32 i = 0; i < n; i++)
                {
                    // Find pivot
                    basic::i32 max_row = i;
                    for (basic::i32 k = i + 1; k < n; k++)
                    {
                        if (math::basic::abs(augmented.at(k, i)) > math::basic::abs(augmented.at(max_row, i)))
                        {
                            max_row = k;
                        };
                    };
                    
                    if (max_row != i)
                    {
                        augmented.swap_rows(i, max_row);
                    };
                    
                    float pivot = augmented.at(i, i);
                    if (math::basic::abs(pivot) < math::constants::FLOAT_EPSILON)
                    {
                        throw("Matrix is singular, cannot compute inverse");
                    };
                    
                    // Scale pivot row
                    for (basic::i32 j = 0; j < 2 * n; j++)
                    {
                        augmented.set(i, j, augmented.at(i, j) / pivot);
                    };
                    
                    // Eliminate column
                    for (basic::i32 k = 0; k < n; k++)
                    {
                        if (k != i)
                        {
                            float factor = augmented.at(k, i);
                            for (basic::i32 j = 0; j < 2 * n; j++)
                            {
                                augmented.set(k, j, augmented.at(k, j) - factor * augmented.at(i, j));
                            };
                        };
                    };
                };
                
                // Extract inverse matrix
                basic::matrix result(n, n);
                for (basic::i32 i = 0; i < n; i++)
                {
                    for (basic::i32 j = 0; j < n; j++)
                    {
                        result.set(i, j, augmented.at(i, j + n));
                    };
                };
                
                return result;
            };
            
            def solve_linear_system(basic::matrix a, basic::matrix b) -> basic::matrix
            {
                if (!a.is_square())
                {
                    throw("Coefficient matrix must be square");
                };
                if (a.rows != b.rows)
                {
                    throw("Coefficient and constant matrices must have same number of rows");
                };
                
                basic::i32 n = a.rows;
                basic::i32 m = b.cols;
                
                // Create augmented matrix [A|B]
                basic::matrix augmented(n, n + m);
                for (basic::i32 i = 0; i < n; i++)
                {
                    for (basic::i32 j = 0; j < n; j++)
                    {
                        augmented.set(i, j, a.at(i, j));
                    };
                    for (basic::i32 j = 0; j < m; j++)
                    {
                        augmented.set(i, j + n, b.at(i, j));
                    };
                };
                
                // Forward elimination
                for (basic::i32 i = 0; i < n; i++)
                {
                    // Find pivot
                    basic::i32 max_row = i;
                    for (basic::i32 k = i + 1; k < n; k++)
                    {
                        if (math::basic::abs(augmented.at(k, i)) > math::basic::abs(augmented.at(max_row, i)))
                        {
                            max_row = k;
                        };
                    };
                    
                    if (max_row != i)
                    {
                        augmented.swap_rows(i, max_row);
                    };
                    
                    float pivot = augmented.at(i, i);
                    if (math::basic::abs(pivot) < math::constants::FLOAT_EPSILON)
                    {
                        throw("System is singular or inconsistent");
                    };
                    
                    // Eliminate below pivot
                    for (basic::i32 k = i + 1; k < n; k++)
                    {
                        float factor = augmented.at(k, i) / pivot;
                        for (basic::i32 j = i; j < n + m; j++)
                        {
                            augmented.set(k, j, augmented.at(k, j) - factor * augmented.at(i, j));
                        };
                    };
                };
                
                // Back substitution
                basic::matrix result(n, m);
                for (basic::i32 col = 0; col < m; col++)
                {
                    for (basic::i32 i = n - 1; i >= 0; i--)
                    {
                        float sum = augmented.at(i, n + col);
                        for (basic::i32 j = i + 1; j < n; j++)
                        {
                            sum -= augmented.at(i, j) * result.at(j, col);
                        };
                        result.set(i, col, sum / augmented.at(i, i));
                    };
                };
                
                return result;
            };
            
            // LU Decomposition
            object lu_decomposition
            {
                basic::matrix L;
                basic::matrix U;
                basic::i32* P; // Permutation vector
                basic::i32 n;
                
                def __init(basic::matrix a) -> this
                {
                    if (!a.is_square())
                    {
                        throw("LU decomposition requires square matrix");
                    };
                    
                    this.n = a.rows;
                    this.L = operations::zeros(this.n, this.n);
                    this.U = operations::zeros(this.n, this.n);
                    // TODO: Allocate permutation vector
                    
                    // Initialize P as identity permutation
                    for (basic::i32 i = 0; i < this.n; i++)
                    {
                        this.P[i] = i;
                    };
                    
                    // Copy A to U
                    this.U.copy_from(a);
                    
                    // Perform LU decomposition with partial pivoting
                    for (basic::i32 i = 0; i < this.n - 1; i++)
                    {
                        // Find pivot
                        basic::i32 max_row = i;
                        for (basic::i32 k = i + 1; k < this.n; k++)
                        {
                            if (math::basic::abs(this.U.at(k, i)) > math::basic::abs(this.U.at(max_row, i)))
                            {
                                max_row = k;
                            };
                        };
                        
                        if (max_row != i)
                        {
                            this.U.swap_rows(i, max_row);
                            this.L.swap_rows(i, max_row);
                            basic::i32 temp = this.P[i];
                            this.P[i] = this.P[max_row];
                            this.P[max_row] = temp;
                        };
                        
                        if (math::basic::abs(this.U.at(i, i)) < math::constants::FLOAT_EPSILON)
                        {
                            throw("Matrix is singular");
                        };
                        
                        // Eliminate and store multipliers in L
                        for (basic::i32 k = i + 1; k < this.n; k++)
                        {
                            float multiplier = this.U.at(k, i) / this.U.at(i, i);
                            this.L.set(k, i, multiplier);
                            
                            for (basic::i32 j = i; j < this.n; j++)
                            {
                                this.U.set(k, j, this.U.at(k, j) - multiplier * this.U.at(i, j));
                            };
                        };
                    };
                    
                    // Set diagonal of L to 1
                    for (basic::i32 i = 0; i < this.n; i++)
                    {
                        this.L.set(i, i, 1.0);
                    };
                    
                    return this;
                };
                
                def __exit() -> void
                {
                    if (this.P is !void)
                    {
                        (void)this.P;
                    };
                    return void;
                };
                
                def solve(basic::matrix b) -> basic::matrix
                {
                    if (b.rows != this.n)
                    {
                        throw("Right-hand side dimension mismatch");
                    };
                    
                    basic::i32 m = b.cols;
                    basic::matrix result(this.n, m);
                    
                    for (basic::i32 col = 0; col < m; col++)
                    {
                        // Apply permutation
                        float y[this.n];
                        for (basic::i32 i = 0; i < this.n; i++)
                        {
                            y[i] = b.at(this.P[i], col);
                        };
                        
                        // Forward substitution: Ly = Pb
                        for (basic::i32 i = 0; i < this.n; i++)
                        {
                            for (basic::i32 j = 0; j < i; j++)
                            {
                                y[i] -= this.L.at(i, j) * y[j];
                            };
                        };
                        
                        // Back substitution: Ux = y
                        for (basic::i32 i = this.n - 1; i >= 0; i--)
                        {
                            float sum = y[i];
                            for (basic::i32 j = i + 1; j < this.n; j++)
                            {
                                sum -= this.U.at(i, j) * result.at(j, col);
                            };
                            result.set(i, col, sum / this.U.at(i, i));
                        };
                    };
                    
                    return result;
                };
                
                def determinant() -> float
                {
                    float det = 1.0;
                    for (basic::i32 i = 0; i < this.n; i++)
                    {
                        det *= this.U.at(i, i);
                    };
                    
                    // Count permutation inversions
                    basic::i32 inversions = 0;
                    for (basic::i32 i = 0; i < this.n; i++)
                    {
                        if (this.P[i] != i)
                        {
                            inversions++;
                        };
                    };
                    
                    return (inversions % 2 == 0) ? det : -det;
                };
            };
            
            // QR Decomposition using Gram-Schmidt process
            object qr_decomposition
            {
                basic::matrix Q;
                basic::matrix R;
                
                def __init(basic::matrix a) -> this
                {
                    basic::i32 m = a.rows;
                    basic::i32 n = a.cols;
                    
                    this.Q = operations::zeros(m, n);
                    this.R = operations::zeros(n, n);
                    
                    // Modified Gram-Schmidt process
                    for (basic::i32 j = 0; j < n; j++)
                    {
                        // Get column j of A
                        basic::matrix v = a.get_col(j);
                        
                        // Orthogonalize against previous columns
                        for (basic::i32 i = 0; i < j; i++)
                        {
                            basic::matrix qi = this.Q.get_col(i);
                            
                            // Compute dot product
                            float dot = 0.0;
                            for (basic::i32 k = 0; k < m; k++)
                            {
                                dot += qi.at(k, 0) * v.at(k, 0);
                            };
                            
                            this.R.set(i, j, dot);
                            
                            // Subtract projection
                            for (basic::i32 k = 0; k < m; k++)
                            {
                                v.set(k, 0, v.at(k, 0) - dot * qi.at(k, 0));
                            };
                        };
                        
                        // Normalize
                        float norm = 0.0;
                        for (basic::i32 k = 0; k < m; k++)
                        {
                            norm += v.at(k, 0) * v.at(k, 0);
                        };
                        norm = math::basic::sqrt(norm);
                        
                        this.R.set(j, j, norm);
                        
                        if (norm > math::constants::FLOAT_EPSILON)
                        {
                            for (basic::i32 k = 0; k < m; k++)
                            {
                                this.Q.set(k, j, v.at(k, 0) / norm);
                            };
                        };
                    };
                    
                    return this;
                };
                
                def __exit() -> void
                {
                    return void;
                };
                
                def solve(basic::matrix b) -> basic::matrix
                {
                    if (b.rows != this.Q.rows)
                    {
                        throw("Right-hand side dimension mismatch");
                    };
                    
                    // Solve QRx = b
                    // First compute Q^T * b
                    basic::matrix qt_b = this.Q.transpose().__mul(b);
                    
                    // Then solve Rx = Q^T * b using back substitution
                    basic::i32 n = this.R.rows;
                    basic::i32 m = qt_b.cols;
                    basic::matrix result(n, m);
                    
                    for (basic::i32 col = 0; col < m; col++)
                    {
                        for (basic::i32 i = n - 1; i >= 0; i--)
                        {
                            float sum = qt_b.at(i, col);
                            for (basic::i32 j = i + 1; j < n; j++)
                            {
                                sum -= this.R.at(i, j) * result.at(j, col);
                            };
                            result.set(i, col, sum / this.R.at(i, i));
                        };
                    };
                    
                    return result;
                };
            };
        };
        
        // ================================================================
        // SPECIALIZED MATRICES
        // ================================================================
        
        namespace specialized
        {
            // Symmetric matrix
            object symmetric_matrix
            {
                float* data;
                basic::i32 n;
                
                def __init(basic::i32 size) -> this
                {
                    this.n = size;
                    basic::i32 storage_size = (size * (size + 1)) / 2;
                    // TODO: Allocate data array of size storage_size
                    return this;
                };
                
                def __exit() -> void
                {
                    if (this.data is !void)
                    {
                        (void)this.data;
                    };
                    return void;
                };
                
                def get_index(basic::i32 i, basic::i32 j) -> basic::i32
                {
                    if (i > j)
                    {
                        basic::i32 temp = i;
                        i = j;
                        j = temp;
                    };
                    return i * this.n - (i * (i + 1)) / 2 + j;
                };
                
                def at(basic::i32 i, basic::i32 j) -> float
                {
                    return this.data[this.get_index(i, j)];
                };
                
                def set(basic::i32 i, basic::i32 j, float value) -> void
                {
                    this.data[this.get_index(i, j)] = value;
                    return void;
                };
                
                def to_matrix() -> basic::matrix
                {
                    basic::matrix result(this.n, this.n);
                    for (basic::i32 i = 0; i < this.n; i++)
                    {
                        for (basic::i32 j = 0; j < this.n; j++)
                        {
                            result.set(i, j, this.at(i, j));
                        };
                    };
                    return result;
                };
            };
            
            // Sparse matrix using coordinate format
            object sparse_matrix
            {
                basic::i32* row_indices;
                basic::i32* col_indices;
                float* values;
                basic::i32 nnz; // Number of non-zero elements
                basic::i32 capacity;
                basic::i32 rows;
                basic::i32 cols;
                
                def __init(basic::i32 r, basic::i32 c, basic::i32 max_nnz) -> this
                {
                    this.rows = r;
                    this.cols = c;
                    this.nnz = 0;
                    this.capacity = max_nnz;
                    // TODO: Allocate arrays
                    return this;
                };
                
                def __exit() -> void
                {
                    if (this.row_indices is !void) { (void)this.row_indices; };
                    if (this.col_indices is !void) { (void)this.col_indices; };
                    if (this.values is !void) { (void)this.values; };
                    return void;
                };
                
                def add_element(basic::i32 row, basic::i32 col, float value) -> void
                {
                    if (this.nnz >= this.capacity)
                    {
                        throw("Sparse matrix capacity exceeded");
                    };
                    if (math::basic::abs(value) < math::constants::FLOAT_EPSILON)
                    {
                        return void; // Don't store zeros
                    };
                    
                    this.row_indices[this.nnz] = row;
                    this.col_indices[this.nnz] = col;
                    this.values[this.nnz] = value;
                    this.nnz++;
                    return void;
                };
                
                def at(basic::i32 row, basic::i32 col) -> float
                {
                    for (basic::i32 i = 0; i < this.nnz; i++)
                    {
                        if (this.row_indices[i] == row and this.col_indices[i] == col)
                        {
                            return this.values[i];
                        };
                    };
                    return 0.0;
                };
                
                def to_matrix() -> basic::matrix
                {
                    basic::matrix result(this.rows, this.cols);
                    result.zero();
                    
                    for (basic::i32 i = 0; i < this.nnz; i++)
                    {
                        result.set(this.row_indices[i], this.col_indices[i], this.values[i]);
                    };
                    
                    return result;
                };
                
                def transpose() -> sparse_matrix
                {
                    sparse_matrix result(this.cols, this.rows, this.capacity);
                    
                    for (basic::i32 i = 0; i < this.nnz; i++)
                    {
                        result.add_element(this.col_indices[i], this.row_indices[i], this.values[i]);
                    };
                    
                    return result;
                };
            };
            
            // Band matrix
            object band_matrix
            {
                float* data;
                basic::i32 n;
                basic::i32 lower_bandwidth;
                basic::i32 upper_bandwidth;
                basic::i32 storage_width;
                
                def __init(basic::i32 size, basic::i32 lower_bw, basic::i32 upper_bw) -> this
                {
                    this.n = size;
                    this.lower_bandwidth = lower_bw;
                    this.upper_bandwidth = upper_bw;
                    this.storage_width = lower_bw + upper_bw + 1;
                    // TODO: Allocate data array of size n * storage_width
                    return this;
                };
                
                def __exit() -> void
                {
                    if (this.data is !void)
                    {
                        (void)this.data;
                    };
                    return void;
                };
                
                def is_valid_index(basic::i32 i, basic::i32 j) -> bool
                {
                    return j >= i - this.lower_bandwidth and j <= i + this.upper_bandwidth;
                };
                
                def at(basic::i32 i, basic::i32 j) -> float
                {
                    if (!this.is_valid_index(i, j))
                    {
                        return 0.0;
                    };
                    basic::i32 band_index = j - i + this.lower_bandwidth;
                    return this.data[i * this.storage_width + band_index];
                };
                
                def set(basic::i32 i, basic::i32 j, float value) -> void
                {
                    if (!this.is_valid_index(i, j))
                    {
                        if (math::basic::abs(value) > math::constants::FLOAT_EPSILON)
                        {
                            throw("Attempting to set non-zero value outside band");
                        };
                        return void;
                    };
                    basic::i32 band_index = j - i + this.lower_bandwidth;
                    this.data[i * this.storage_width + band_index] = value;
                    return void;
                };
                
                def to_matrix() -> basic::matrix
                {
                    basic::matrix result(this.n, this.n);
                    result.zero();
                    
                    for (basic::i32 i = 0; i < this.n; i++)
                    {
                        for (basic::i32 j = 0; j < this.n; j++)
                        {
                            if (this.is_valid_index(i, j))
                            {
                                result.set(i, j, this.at(i, j));
                            };
                        };
                    };
                    
                    return result;
                };
            };
            
            // Tridiagonal matrix
            object tridiagonal_matrix
            {
                float* lower; // Lower diagonal
                float* main;  // Main diagonal
                float* upper; // Upper diagonal
                basic::i32 n;
                
                def __init(basic::i32 size) -> this
                {
                    this.n = size;
                    // TODO: Allocate arrays of appropriate sizes
                    return this;
                };
                
                def __exit() -> void
                {
                    if (this.lower is !void) { (void)this.lower; };
                    if (this.main is !void) { (void)this.main; };
                    if (this.upper is !void) { (void)this.upper; };
                    return void;
                };
                
                def at(basic::i32 i, basic::i32 j) -> float
                {
                    if (i == j) { return this.main[i]; };
                    if (i == j + 1) { return this.lower[i]; };
                    if (i == j - 1) { return this.upper[i]; };
                    return 0.0;
                };
                
                def set(basic::i32 i, basic::i32 j, float value) -> void
                {
                    if (i == j) { this.main[i] = value; return void; };
                    if (i == j + 1) { this.lower[i] = value; return void; };
                    if (i == j - 1) { this.upper[i] = value; return void; };
                    if (math::basic::abs(value) > math::constants::FLOAT_EPSILON)
                    {
                        throw("Attempting to set non-zero value outside tridiagonal structure");
                    };
                    return void;
                };
                
                def solve(float b[]) -> void
                {
                    // Thomas algorithm for tridiagonal systems
                    float c_prime[this.n];
                    float d_prime[this.n];
                    
                    // Forward sweep
                    c_prime[0] = this.upper[0] / this.main[0];
                    d_prime[0] = b[0] / this.main[0];
                    
                    for (basic::i32 i = 1; i < this.n; i++)
                    {
                        float denominator = this.main[i] - this.lower[i] * c_prime[i - 1];
                        if (i < this.n - 1)
                        {
                            c_prime[i] = this.upper[i] / denominator;
                        };
                        d_prime[i] = (b[i] - this.lower[i] * d_prime[i - 1]) / denominator;
                    };
                    
                    // Back substitution
                    b[this.n - 1] = d_prime[this.n - 1];
                    for (basic::i32 i = this.n - 2; i >= 0; i--)
                    {
                        b[i] = d_prime[i] - c_prime[i] * b[i + 1];
                    };
                    
                    return void;
                };
                
                def to_matrix() -> basic::matrix
                {
                    basic::matrix result(this.n, this.n);
                    result.zero();
                    
                    for (basic::i32 i = 0; i < this.n; i++)
                    {
                        result.set(i, i, this.main[i]);
                        if (i > 0)
                        {
                            result.set(i, i - 1, this.lower[i]);
                        };
                        if (i < this.n - 1)
                        {
                            result.set(i, i + 1, this.upper[i]);
                        };
                    };
                    
                    return result;
                };
            };
        };
        
        // ================================================================
        // EIGENVALUE AND EIGENVECTOR COMPUTATIONS
        // ================================================================
        
        namespace eigenvalue
        {
            // Power iteration for largest eigenvalue
            def power_iteration(basic::matrix a, basic::i32 max_iterations, float tolerance) -> float
            {
                if (!a.is_square())
                {
                    throw("Power iteration requires square matrix");
                };
                
                basic::i32 n = a.rows;
                basic::matrix x(n, 1);
                
                // Initialize with random vector
                for (basic::i32 i = 0; i < n; i++)
                {
                    x.set(i, 0, math::random::rand_float());
                };
                
                float eigenvalue = 0.0;
                
                for (basic::i32 iter = 0; iter < max_iterations; iter++)
                {
                    // y = A * x
                    basic::matrix y = a.__mul(x);
                    
                    // Find largest component for normalization
                    float max_component = 0.0;
                    for (basic::i32 i = 0; i < n; i++)
                    {
                        if (math::basic::abs(y.at(i, 0)) > math::basic::abs(max_component))
                        {
                            max_component = y.at(i, 0);
                        };
                    };
                    
                    if (math::basic::abs(max_component) < math::constants::FLOAT_EPSILON)
                    {
                        throw("Power iteration failed to converge");
                    };
                    
                    // Normalize
                    for (basic::i32 i = 0; i < n; i++)
                    {
                        x.set(i, 0, y.at(i, 0) / max_component);
                    };
                    
                    // Check convergence
                    if (iter > 0 and math::basic::abs(eigenvalue - max_component) < tolerance)
                    {
                        return max_component;
                    };
                    
                    eigenvalue = max_component;
                };
                
                return eigenvalue;
            };
            
            // Inverse power iteration for smallest eigenvalue
            def inverse_power_iteration(basic::matrix a, basic::i32 max_iterations, float tolerance) -> float
            {
                basic::matrix a_inv = operations::inverse(a);
                float largest_inv_eigenvalue = power_iteration(a_inv, max_iterations, tolerance);
                return 1.0 / largest_inv_eigenvalue;
            };
            
            // QR algorithm for eigenvalues (simplified version)
            object qr_eigenvalue_solver
            {
                basic::matrix eigenvalues;
                basic::i32 n;
                
                def __init(basic::matrix a) -> this
                {
                    if (!a.is_square())
                    {
                        throw("QR eigenvalue solver requires square matrix");
                    };
                    
                    this.n = a.rows;
                    this.eigenvalues = operations::zeros(this.n, 1);
                    
                    basic::matrix work_matrix();
                    work_matrix.copy_from(a);
                    
                    // Simplified QR iteration
                    for (basic::i32 iter = 0; iter < 100; iter++)
                    {
                        operations::qr_decomposition qr(work_matrix);
                        work_matrix = qr.R.__mul(qr.Q);
                        
                        // Check for convergence (simplified)
                        bool converged = true;
                        for (basic::i32 i = 0; i < this.n - 1; i++)
                        {
                            for (basic::i32 j = i + 1; j < this.n; j++)
                            {
                                if (math::basic::abs(work_matrix.at(i, j)) > math::constants::FLOAT_EPSILON)
                                {
                                    converged = false;
                                };
                            };
                        };
                        
                        if (converged) { break; };
                    };
                    
                    // Extract eigenvalues from diagonal
                    for (basic::i32 i = 0; i < this.n; i++)
                    {
                        this.eigenvalues.set(i, 0, work_matrix.at(i, i));
                    };
                    
                    return this;
                };
                
                def __exit() -> void
                {
                    return void;
                };
                
                def get_eigenvalue(basic::i32 index) -> float
                {
                    if (index >= this.n or index < 0)
                    {
                        throw("Eigenvalue index out of bounds");
                    };
                    return this.eigenvalues.at(index, 0);
                };
                
                def largest_eigenvalue() -> float
                {
                    float max_val = this.eigenvalues.at(0, 0);
                    for (basic::i32 i = 1; i < this.n; i++)
                    {
                        max_val = math::basic::max(max_val, this.eigenvalues.at(i, 0));
                    };
                    return max_val;
                };
                
                def smallest_eigenvalue() -> float
                {
                    float min_val = this.eigenvalues.at(0, 0);
                    for (basic::i32 i = 1; i < this.n; i++)
                    {
                        min_val = math::basic::min(min_val, this.eigenvalues.at(i, 0));
                    };
                    return min_val;
                };
            };
        };
        
        // ================================================================
        // MATRIX UTILITIES
        // ================================================================
        
        namespace utilities
        {
            // Matrix condition number
            def condition_number(basic::matrix a) -> float
            {
                eigenvalue::qr_eigenvalue_solver solver(a);
                float max_eigenval = solver.largest_eigenvalue();
                float min_eigenval = solver.smallest_eigenvalue();
                
                if (math::basic::abs(min_eigenval) < math::constants::FLOAT_EPSILON)
                {
                    return math::constants::INFINITY;
                };
                
                return math::basic::abs(max_eigenval / min_eigenval);
            };
            
            // Matrix rank (using SVD approximation)
            def rank(basic::matrix a, float tolerance) -> basic::i32
            {
                // Simplified rank computation using row reduction
                basic::matrix work_matrix();
                work_matrix.copy_from(a);
                
                basic::i32 rank = 0;
                basic::i32 rows = work_matrix.rows;
                basic::i32 cols = work_matrix.cols;
                
                for (basic::i32 col = 0; col < cols and rank < rows; col++)
                {
                    // Find pivot
                    basic::i32 pivot_row = rank;
                    for (basic::i32 row = rank + 1; row < rows; row++)
                    {
                        if (math::basic::abs(work_matrix.at(row, col)) > math::basic::abs(work_matrix.at(pivot_row, col)))
                        {
                            pivot_row = row;
                        };
                    };
                    
                    if (math::basic::abs(work_matrix.at(pivot_row, col)) < tolerance)
                    {
                        continue; // Skip this column
                    };
                    
                    // Swap rows
                    if (pivot_row != rank)
                    {
                        work_matrix.swap_rows(rank, pivot_row);
                    };
                    
                    // Eliminate below
                    for (basic::i32 row = rank + 1; row < rows; row++)
                    {
                        float factor = work_matrix.at(row, col) / work_matrix.at(rank, col);
                        for (basic::i32 j = col; j < cols; j++)
                        {
                            work_matrix.set(row, j, work_matrix.at(row, j) - factor * work_matrix.at(rank, j));
                        };
                    };
                    
                    rank++;
                };
                
                return rank;
            };
            
            // Check if matrix is positive definite
            def is_positive_definite(basic::matrix a) -> bool
            {
                if (!a.is_square())
                {
                    return false;
                };
                
                try
                {
                    eigenvalue::qr_eigenvalue_solver solver(a);
                    for (basic::i32 i = 0; i < a.rows; i++)
                    {
                        if (solver.get_eigenvalue(i) <= 0.0)
                        {
                            return false;
                        };
                    };
                    return true;
                }
                catch (basic::string error)
                {
                    return false;
                };
            };
            
            // Check if matrix is symmetric
            def is_symmetric(basic::matrix a, float tolerance) -> bool
            {
                if (!a.is_square())
                {
                    return false;
                };
                
                for (basic::i32 i = 0; i < a.rows; i++)
                {
                    for (basic::i32 j = 0; j < a.cols; j++)
                    {
                        if (math::basic::abs(a.at(i, j) - a.at(j, i)) > tolerance)
                        {
                            return false;
                        };
                    };
                };
                
                return true;
            };
            
            // Check if matrix is orthogonal
            def is_orthogonal(basic::matrix a, float tolerance) -> bool
            {
                if (!a.is_square())
                {
                    return false;
                };
                
                basic::matrix at = a.transpose();
                basic::matrix product = a.__mul(at);
                basic::matrix identity = operations::identity(a.rows);
                
                return product.__ee(identity);
            };
            
            // Matrix pseudoinverse using SVD (simplified)
            def pseudoinverse(basic::matrix a, float tolerance) -> basic::matrix
            {
                // For now, use Moore-Penrose pseudoinverse formula for full rank case
                if (a.rows >= a.cols)
                {
                    // Left pseudoinverse: (A^T A)^-1 A^T
                    basic::matrix at = a.transpose();
                    basic::matrix ata = at.__mul(a);
                    basic::matrix ata_inv = operations::inverse(ata);
                    return ata_inv.__mul(at);
                }
                else
                {
                    // Right pseudoinverse: A^T (A A^T)^-1
                    basic::matrix at = a.transpose();
                    basic::matrix aat = a.__mul(at);
                    basic::matrix aat_inv = operations::inverse(aat);
                    return at.__mul(aat_inv);
                };
            };
            
            // Kronecker product
            def kronecker_product(basic::matrix a, basic::matrix b) -> basic::matrix
            {
                basic::i32 result_rows = a.rows * b.rows;
                basic::i32 result_cols = a.cols * b.cols;
                basic::matrix result(result_rows, result_cols);
                
                for (basic::i32 i = 0; i < a.rows; i++)
                {
                    for (basic::i32 j = 0; j < a.cols; j++)
                    {
                        float a_ij = a.at(i, j);
                        for (basic::i32 k = 0; k < b.rows; k++)
                        {
                            for (basic::i32 l = 0; l < b.cols; l++)
                            {
                                result.set(i * b.rows + k, j * b.cols + l, a_ij * b.at(k, l));
                            };
                        };
                    };
                };
                
                return result;
            };
            
            // Vectorization (reshape matrix to column vector)
            def vectorize(basic::matrix a) -> basic::matrix
            {
                basic::matrix result(a.rows * a.cols, 1);
                for (basic::i32 i = 0; i < a.rows; i++)
                {
                    for (basic::i32 j = 0; j < a.cols; j++)
                    {
                        result.set(i * a.cols + j, 0, a.at(i, j));
                    };
                };
                return result;
            };
            
            // Reshape matrix
            def reshape(basic::matrix a, basic::i32 new_rows, basic::i32 new_cols) -> basic::matrix
            {
                if (a.get_size() != new_rows * new_cols)
                {
                    throw("Total number of elements must remain the same");
                };
                
                basic::matrix result(new_rows, new_cols);
                for (basic::i32 i = 0; i < a.get_size(); i++)
                {
                    basic::i32 old_row = i / a.cols;
                    basic::i32 old_col = i % a.cols;
                    basic::i32 new_row = i / new_cols;
                    basic::i32 new_col = i % new_cols;
                    result.set(new_row, new_col, a.at(old_row, old_col));
                };
                
                return result;
            };
            
            // Block matrix construction
            def block_matrix(basic::matrix blocks[][], basic::i32 block_rows, basic::i32 block_cols) -> basic::matrix
            {
                // Calculate total dimensions
                basic::i32 total_rows = 0;
                basic::i32 total_cols = 0;
                
                for (basic::i32 i = 0; i < block_rows; i++)
                {
                    total_rows += blocks[i][0].rows;
                };
                
                for (basic::i32 j = 0; j < block_cols; j++)
                {
                    total_cols += blocks[0][j].cols;
                };
                
                basic::matrix result(total_rows, total_cols);
                
                basic::i32 row_offset = 0;
                for (basic::i32 i = 0; i < block_rows; i++)
                {
                    basic::i32 col_offset = 0;
                    for (basic::i32 j = 0; j < block_cols; j++)
                    {
                        basic::matrix block = blocks[i][j];
                        for (basic::i32 r = 0; r < block.rows; r++)
                        {
                            for (basic::i32 c = 0; c < block.cols; c++)
                            {
                                result.set(row_offset + r, col_offset + c, block.at(r, c));
                            };
                        };
                        col_offset += block.cols;
                    };
                    row_offset += blocks[i][0].rows;
                };
                
                return result;
            };
        };
    };
};