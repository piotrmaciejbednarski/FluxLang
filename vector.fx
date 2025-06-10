// ================================================================
// FLUX VECTORS LIBRARY
// vector.fx - Comprehensive vector operations and utilities
// ================================================================

import "types.fx";
import "math.fx";

namespace standard
{
    namespace vector
    {
        using standard::types::basic;
        using standard::math;
        
        // ================================================================
        // BASIC VECTOR TYPES
        // ================================================================
        
        namespace basic
        {
            // Generic n-dimensional vector
            object vector
            {
                float* adata;
                basic::i32 dimensions;
                basic::i32 capacity;
                
                def __init(basic::i32 dim) -> this
                {
                    this.dimensions = dim;
                    this.capacity = dim;
                    // TODO: Allocate data array of size dim
                    for (basic::i32 i = 0; i < this.dimensions; i++)
                    {
                        this.adata[i] = 0.0;
                    };
                    return this;
                };
                
                def __init() -> this
                {
                    this.dimensions = 0;
                    this.capacity = 0;
                    this.adata = void;
                    return this;
                };
                
                def __exit() -> void
                {
                    if (this.adata is !void)
                    {
                        (void)this.adata;
                    };
                    return void;
                };
                
                def get_dimensions() -> basic::i32
                {
                    return this.dimensions;
                };
                
                def size() -> basic::i32
                {
                    return this.dimensions;
                };
                
                def is_empty() -> bool
                {
                    return this.dimensions == 0;
                };
                
                def at(basic::i32 index) -> float
                {
                    if (index >= this.dimensions or index < 0)
                    {
                        throw("Vector index out of bounds");
                    };
                    return this.adata[index];
                };
                
                def set(basic::i32 index, float value) -> void
                {
                    if (index >= this.dimensions or index < 0)
                    {
                        throw("Vector index out of bounds");
                    };
                    this.adata[index] = value;
                    return void;
                };
                
                def resize(basic::i32 new_dim) -> void
                {
                    if (new_dim > this.capacity)
                    {
                        if (this.adata is !void)
                        {
                            (void)this.adata;
                        };
                        // TODO: Allocate new data array
                        this.capacity = new_dim;
                    };
                    
                    // Initialize new elements to zero
                    for (basic::i32 i = this.dimensions; i < new_dim; i++)
                    {
                        this.adata[i] = 0.0;
                    };
                    
                    this.dimensions = new_dim;
                    return void;
                };
                
                def fill(float value) -> void
                {
                    for (basic::i32 i = 0; i < this.dimensions; i++)
                    {
                        this.adata[i] = value;
                    };
                    return void;
                };
                
                def zero() -> void
                {
                    this.fill(0.0);
                    return void;
                };
                
                def copy_from(vector other) -> void
                {
                    this.resize(other.dimensions);
                    for (basic::i32 i = 0; i < this.dimensions; i++)
                    {
                        this.adata[i] = other.adata[i];
                    };
                    return void;
                };
                
                def __eq(vector other) -> void
                {
                    this.copy_from(other);
                    return void;
                };
                
                def __ee(vector other) -> bool
                {
                    if (this.dimensions != other.dimensions)
                    {
                        return false;
                    };
                    
                    for (basic::i32 i = 0; i < this.dimensions; i++)
                    {
                        if (math::basic::abs(this.adata[i] - other.adata[i]) > math::constants::FLOAT_EPSILON)
                        {
                            return false;
                        };
                    };
                    
                    return true;
                };
                
                def __ne(vector other) -> bool
                {
                    return !(this.__ee(other));
                };
                
                def __add(vector other) -> vector
                {
                    if (this.dimensions != other.dimensions)
                    {
                        throw("Vector dimensions must match for addition");
                    };
                    
                    vector result(this.dimensions);
                    for (basic::i32 i = 0; i < this.dimensions; i++)
                    {
                        result.adata[i] = this.adata[i] + other.adata[i];
                    };
                    
                    return result;
                };
                
                def __sub(vector other) -> vector
                {
                    if (this.dimensions != other.dimensions)
                    {
                        throw("Vector dimensions must match for subtraction");
                    };
                    
                    vector result(this.dimensions);
                    for (basic::i32 i = 0; i < this.dimensions; i++)
                    {
                        result.adata[i] = this.adata[i] - other.adata[i];
                    };
                    
                    return result;
                };
                
                def __mul(float scalar) -> vector
                {
                    vector result(this.dimensions);
                    for (basic::i32 i = 0; i < this.dimensions; i++)
                    {
                        result.adata[i] = this.adata[i] * scalar;
                    };
                    
                    return result;
                };
                
                def __div(float scalar) -> vector
                {
                    if (math::basic::abs(scalar) < math::constants::FLOAT_EPSILON)
                    {
                        throw("Division by zero in vector scalar division");
                    };
                    
                    vector result(this.dimensions);
                    for (basic::i32 i = 0; i < this.dimensions; i++)
                    {
                        result.adata[i] = this.adata[i] / scalar;
                    };
                    
                    return result;
                };
                
                def __iadd(vector other) -> this
                {
                    if (this.dimensions != other.dimensions)
                    {
                        throw("Vector dimensions must match for in-place addition");
                    };
                    
                    for (basic::i32 i = 0; i < this.dimensions; i++)
                    {
                        this.adata[i] += other.adata[i];
                    };
                    
                    return this;
                };
                
                def __isub(vector other) -> this
                {
                    if (this.dimensions != other.dimensions)
                    {
                        throw("Vector dimensions must match for in-place subtraction");
                    };
                    
                    for (basic::i32 i = 0; i < this.dimensions; i++)
                    {
                        this.adata[i] -= other.adata[i];
                    };
                    
                    return this;
                };
                
                def __imul(float scalar) -> this
                {
                    for (basic::i32 i = 0; i < this.dimensions; i++)
                    {
                        this.adata[i] *= scalar;
                    };
                    
                    return this;
                };
                
                def __idiv(float scalar) -> this
                {
                    if (math::basic::abs(scalar) < math::constants::FLOAT_EPSILON)
                    {
                        throw("Division by zero in vector in-place scalar division");
                    };
                    
                    for (basic::i32 i = 0; i < this.dimensions; i++)
                    {
                        this.adata[i] /= scalar;
                    };
                    
                    return this;
                };
                
                def negate() -> vector
                {
                    return this.__mul(-1.0);
                };
                
                def dot(vector other) -> float
                {
                    if (this.dimensions != other.dimensions)
                    {
                        throw("Vector dimensions must match for dot product");
                    };
                    
                    float result = 0.0;
                    for (basic::i32 i = 0; i < this.dimensions; i++)
                    {
                        result += this.adata[i] * other.adata[i];
                    };
                    
                    return result;
                };
                
                def magnitude_squared() -> float
                {
                    return this.dot(this);
                };
                
                def magnitude() -> float
                {
                    return math::basic::sqrt(this.magnitude_squared());
                };
                
                def length() -> float
                {
                    return this.magnitude();
                };
                
                def normalize() -> vector
                {
                    float mag = this.magnitude();
                    if (mag < math::constants::FLOAT_EPSILON)
                    {
                        throw("Cannot normalize zero vector");
                    };
                    
                    return this.__div(mag);
                };
                
                def normalize_in_place() -> void
                {
                    float mag = this.magnitude();
                    if (mag < math::constants::FLOAT_EPSILON)
                    {
                        throw("Cannot normalize zero vector");
                    };
                    
                    for (basic::i32 i = 0; i < this.dimensions; i++)
                    {
                        this.adata[i] /= mag;
                    };
                    return void;
                };
                
                def is_normalized(float tolerance) -> bool
                {
                    float mag = this.magnitude();
                    return math::basic::abs(mag - 1.0) <= tolerance;
                };
                
                def is_zero(float tolerance) -> bool
                {
                    return this.magnitude() <= tolerance;
                };
                
                def distance_to(vector other) -> float
                {
                    return this.__sub(other).magnitude();
                };
                
                def angle_to(vector other) -> float
                {
                    if (this.dimensions != other.dimensions)
                    {
                        throw("Vector dimensions must match for angle calculation");
                    };
                    
                    float dot_product = this.dot(other);
                    float mag_product = this.magnitude() * other.magnitude();
                    
                    if (mag_product < math::constants::FLOAT_EPSILON)
                    {
                        throw("Cannot compute angle with zero vector");
                    };
                    
                    float cos_angle = dot_product / mag_product;
                    // Clamp to valid range for acos
                    cos_angle = math::basic::clamp(cos_angle, -1.0, 1.0);
                    
                    return math::trigonometric::acos(cos_angle);
                };
                
                def project_onto(vector other) -> vector
                {
                    if (this.dimensions != other.dimensions)
                    {
                        throw("Vector dimensions must match for projection");
                    };
                    
                    float other_mag_sq = other.magnitude_squared();
                    if (other_mag_sq < math::constants::FLOAT_EPSILON)
                    {
                        throw("Cannot project onto zero vector");
                    };
                    
                    float scalar_proj = this.dot(other) / other_mag_sq;
                    return other.__mul(scalar_proj);
                };
                
                def reflect(vector normal) -> vector
                {
                    if (this.dimensions != normal.dimensions)
                    {
                        throw("Vector dimensions must match for reflection");
                    };
                    
                    // v_reflected = v - 2 * (v · n) * n
                    float dot_product = this.dot(normal);
                    vector reflection_component = normal.__mul(2.0 * dot_product);
                    return this.__sub(reflection_component);
                };
                
                def lerp(vector other, float t) -> vector
                {
                    if (this.dimensions != other.dimensions)
                    {
                        throw("Vector dimensions must match for interpolation");
                    };
                    
                    // result = (1 - t) * this + t * other
                    vector result(this.dimensions);
                    for (basic::i32 i = 0; i < this.dimensions; i++)
                    {
                        result.adata[i] = (1.0 - t) * this.adata[i] + t * other.adata[i];
                    };
                    
                    return result;
                };
                
                def slerp(vector other, float t) -> vector
                {
                    if (this.dimensions != other.dimensions)
                    {
                        throw("Vector dimensions must match for spherical interpolation");
                    };
                    
                    float dot_product = this.dot(other);
                    dot_product = math::basic::clamp(dot_product, -1.0, 1.0);
                    
                    float angle = math::trigonometric::acos(math::basic::abs(dot_product));
                    
                    if (angle < math::constants::FLOAT_EPSILON)
                    {
                        // Vectors are nearly parallel, use linear interpolation
                        return this.lerp(other, t);
                    };
                    
                    float sin_angle = math::trigonometric::sin(angle);
                    float factor1 = math::trigonometric::sin((1.0 - t) * angle) / sin_angle;
                    float factor2 = math::trigonometric::sin(t * angle) / sin_angle;
                    
                    vector result = this * factor1;
                    result += other * factor2;
                    return result;
                };
                
                def component_max() -> float
                {
                    if (this.dimensions == 0)
                    {
                        throw("Cannot find max of empty vector");
                    };
                    
                    float max_val = this.adata[0];
                    for (basic::i32 i = 1; i < this.dimensions; i++)
                    {
                        max_val = math::basic::max(max_val, this.adata[i]);
                    };
                    
                    return max_val;
                };
                
                def component_min() -> float
                {
                    if (this.dimensions == 0)
                    {
                        throw("Cannot find min of empty vector");
                    };
                    
                    float min_val = this.adata[0];
                    for (basic::i32 i = 1; i < this.dimensions; i++)
                    {
                        min_val = math::basic::min(min_val, this.adata[i]);
                    };
                    
                    return min_val;
                };
                
                def component_sum() -> float
                {
                    float sum = 0.0;
                    for (basic::i32 i = 0; i < this.dimensions; i++)
                    {
                        sum += this.adata[i];
                    };
                    return sum;
                };
                
                def component_product() -> float
                {
                    if (this.dimensions == 0) { return 0.0; };
                    
                    float product = 1.0;
                    for (basic::i32 i = 0; i < this.dimensions; i++)
                    {
                        product *= this.adata[i];
                    };
                    return product;
                };
                
                def abs() -> vector
                {
                    vector result(this.dimensions);
                    for (basic::i32 i = 0; i < this.dimensions; i++)
                    {
                        result.adata[i] = math::basic::abs(this.adata[i]);
                    };
                    return result;
                };
            };
            
            // Fixed-size 2D vector for performance
            object vector2
            {
                float x, y;
                
                def __init() -> this
                {
                    this.x = 0.0;
                    this.y = 0.0;
                    return this;
                };
                
                def __init(float x_val, float y_val) -> this
                {
                    this.x = x_val;
                    this.y = y_val;
                    return this;
                };
                
                def __exit() -> void
                {
                    return void;
                };
                
                def at(basic::i32 index) -> float
                {
                    if (index == 0) { return this.x; };
                    if (index == 1) { return this.y; };
                    throw("Vector2 index out of bounds");
                };
                
                def set(basic::i32 index, float value) -> void
                {
                    if (index == 0) { this.x = value; return void; };
                    if (index == 1) { this.y = value; return void; };
                    throw("Vector2 index out of bounds");
                    return void;
                };
                
                def __add(vector2 other) -> vector2
                {
                    return vector2(this.x + other.x, this.y + other.y);
                };
                
                def __sub(vector2 other) -> vector2
                {
                    return vector2(this.x - other.x, this.y - other.y);
                };
                
                def __mul(float scalar) -> vector2
                {
                    return vector2(this.x * scalar, this.y * scalar);
                };
                
                def __div(float scalar) -> vector2
                {
                    if (math::basic::abs(scalar) < math::constants::FLOAT_EPSILON)
                    {
                        throw("Division by zero in vector2 scalar division");
                    };
                    return vector2(this.x / scalar, this.y / scalar);
                };
                
                def __iadd(vector2 other) -> this
                {
                    this.x += other.x;
                    this.y += other.y;
                    return this;
                };
                
                def __isub(vector2 other) -> this
                {
                    this.x -= other.x;
                    this.y -= other.y;
                    return this;
                };
                
                def __imul(float scalar) -> this
                {
                    this.x *= scalar;
                    this.y *= scalar;
                    return this;
                };
                
                def __idiv(float scalar) -> this
                {
                    if (math::basic::abs(scalar) < math::constants::FLOAT_EPSILON)
                    {
                        throw("Division by zero in vector2 in-place scalar division");
                    };
                    this.x /= scalar;
                    this.y /= scalar;
                    return this;
                };
                
                def dot(vector2 other) -> float
                {
                    return this.x * other.x + this.y * other.y;
                };
                
                def cross(vector2 other) -> float
                {
                    return this.x * other.y - this.y * other.x;
                };
                
                def magnitude_squared() -> float
                {
                    return this.x * this.x + this.y * this.y;
                };
                
                def magnitude() -> float
                {
                    return math::basic::sqrt(this.magnitude_squared());
                };
                
                def normalize() -> vector2
                {
                    float mag = this.magnitude();
                    if (mag < math::constants::FLOAT_EPSILON)
                    {
                        throw("Cannot normalize zero vector2");
                    };
                    return vector2(this.x / mag, this.y / mag);
                };
                
                def perpendicular() -> vector2
                {
                    return vector2(-this.y, this.x);
                };
                
                def angle() -> float
                {
                    return math::trigonometric::atan2(this.y, this.x);
                };
                
                def rotate(float angle) -> vector2
                {
                    float cos_a = math::trigonometric::cos(angle);
                    float sin_a = math::trigonometric::sin(angle);
                    return vector2(this.x * cos_a - this.y * sin_a,
                                 this.x * sin_a + this.y * cos_a);
                };
                
                def to_vector() -> vector
                {
                    vector result(2);
                    result.set(0, this.x);
                    result.set(1, this.y);
                    return result;
                };
            };
            
            // Fixed-size 3D vector for performance
            object vector3
            {
                float x, y, z;
                
                def __init() -> this
                {
                    this.x = 0.0;
                    this.y = 0.0;
                    this.z = 0.0;
                    return this;
                };
                
                def __init(float x_val, float y_val, float z_val) -> this
                {
                    this.x = x_val;
                    this.y = y_val;
                    this.z = z_val;
                    return this;
                };
                
                def __exit() -> void
                {
                    return void;
                };
                
                def at(basic::i32 index) -> float
                {
                    if (index == 0) { return this.x; };
                    if (index == 1) { return this.y; };
                    if (index == 2) { return this.z; };
                    throw("Vector3 index out of bounds");
                };
                
                def set(basic::i32 index, float value) -> void
                {
                    if (index == 0) { this.x = value; return void; };
                    if (index == 1) { this.y = value; return void; };
                    if (index == 2) { this.z = value; return void; };
                    throw("Vector3 index out of bounds");
                    return void;
                };
                
                def __add(vector3 other) -> vector3
                {
                    return vector3(this.x + other.x, this.y + other.y, this.z + other.z);
                };
                
                def __sub(vector3 other) -> vector3
                {
                    return vector3(this.x - other.x, this.y - other.y, this.z - other.z);
                };
                
                def __mul(float scalar) -> vector3
                {
                    return vector3(this.x * scalar, this.y * scalar, this.z * scalar);
                };
                
                def __div(float scalar) -> vector3
                {
                    if (math::basic::abs(scalar) < math::constants::FLOAT_EPSILON)
                    {
                        throw("Division by zero in vector3 scalar division");
                    };
                    return vector3(this.x / scalar, this.y / scalar, this.z / scalar);
                };
                
                def __iadd(vector3 other) -> this
                {
                    this.x += other.x;
                    this.y += other.y;
                    this.z += other.z;
                    return this;
                };
                
                def __isub(vector3 other) -> this
                {
                    this.x -= other.x;
                    this.y -= other.y;
                    this.z -= other.z;
                    return this;
                };
                
                def __imul(float scalar) -> this
                {
                    this.x *= scalar;
                    this.y *= scalar;
                    this.z *= scalar;
                    return this;
                };
                
                def __idiv(float scalar) -> this
                {
                    if (math::basic::abs(scalar) < math::constants::FLOAT_EPSILON)
                    {
                        throw("Division by zero in vector3 in-place scalar division");
                    };
                    this.x /= scalar;
                    this.y /= scalar;
                    this.z /= scalar;
                    return this;
                };
                
                def dot(vector3 other) -> float
                {
                    return this.x * other.x + this.y * other.y + this.z * other.z;
                };
                
                def cross(vector3 other) -> vector3
                {
                    return vector3(this.y * other.z - this.z * other.y,
                                 this.z * other.x - this.x * other.z,
                                 this.x * other.y - this.y * other.x);
                };
                
                def magnitude_squared() -> float
                {
                    return this.x * this.x + this.y * this.y + this.z * this.z;
                };
                
                def magnitude() -> float
                {
                    return math::basic::sqrt(this.magnitude_squared());
                };
                
                def normalize() -> vector3
                {
                    float mag = this.magnitude();
                    if (mag < math::constants::FLOAT_EPSILON)
                    {
                        throw("Cannot normalize zero vector3");
                    };
                    return vector3(this.x / mag, this.y / mag, this.z / mag);
                };
                
                def triple_product(vector3 b, vector3 c) -> float
                {
                    return this.dot(b.cross(c));
                };
                
                def to_vector() -> vector
                {
                    vector result(3);
                    result.set(0, this.x);
                    result.set(1, this.y);
                    result.set(2, this.z);
                    return result;
                };
            };
            
            // Fixed-size 4D vector (useful for graphics/quaternions)
            object vector4
            {
                float x, y, z, w;
                
                def __init() -> this
                {
                    this.x = 0.0;
                    this.y = 0.0;
                    this.z = 0.0;
                    this.w = 0.0;
                    return this;
                };
                
                def __init(float x_val, float y_val, float z_val, float w_val) -> this
                {
                    this.x = x_val;
                    this.y = y_val;
                    this.z = z_val;
                    this.w = w_val;
                    return this;
                };
                
                def __exit() -> void
                {
                    return void;
                };
                
                def at(basic::i32 index) -> float
                {
                    if (index == 0) { return this.x; };
                    if (index == 1) { return this.y; };
                    if (index == 2) { return this.z; };
                    if (index == 3) { return this.w; };
                    throw("Vector4 index out of bounds");
                };
                
                def set(basic::i32 index, float value) -> void
                {
                    if (index == 0) { this.x = value; return void; };
                    if (index == 1) { this.y = value; return void; };
                    if (index == 2) { this.z = value; return void; };
                    if (index == 3) { this.w = value; return void; };
                    throw("Vector4 index out of bounds");
                    return void;
                };
                
                def __add(vector4 other) -> vector4
                {
                    return vector4(this.x + other.x, this.y + other.y, 
                                 this.z + other.z, this.w + other.w);
                };
                
                def __sub(vector4 other) -> vector4
                {
                    return vector4(this.x - other.x, this.y - other.y, 
                                 this.z - other.z, this.w - other.w);
                };
                
                def __mul(float scalar) -> vector4
                {
                    return vector4(this.x * scalar, this.y * scalar, 
                                 this.z * scalar, this.w * scalar);
                };
                
                def __div(float scalar) -> vector4
                {
                    if (math::basic::abs(scalar) < math::constants::FLOAT_EPSILON)
                    {
                        throw("Division by zero in vector4 scalar division");
                    };
                    return vector4(this.x / scalar, this.y / scalar, 
                                 this.z / scalar, this.w / scalar);
                };
                
                def __iadd(vector4 other) -> this
                {
                    this.x += other.x;
                    this.y += other.y;
                    this.z += other.z;
                    this.w += other.w;
                    return this;
                };
                
                def __isub(vector4 other) -> this
                {
                    this.x -= other.x;
                    this.y -= other.y;
                    this.z -= other.z;
                    this.w -= other.w;
                    return this;
                };
                
                def __imul(float scalar) -> this
                {
                    this.x *= scalar;
                    this.y *= scalar;
                    this.z *= scalar;
                    this.w *= scalar;
                    return this;
                };
                
                def __idiv(float scalar) -> this
                {
                    if (math::basic::abs(scalar) < math::constants::FLOAT_EPSILON)
                    {
                        throw("Division by zero in vector4 in-place scalar division");
                    };
                    this.x /= scalar;
                    this.y /= scalar;
                    this.z /= scalar;
                    this.w /= scalar;
                    return this;
                };
                
                def dot(vector4 other) -> float
                {
                    return this.x * other.x + this.y * other.y + 
                           this.z * other.z + this.w * other.w;
                };
                
                def magnitude_squared() -> float
                {
                    return this.x * this.x + this.y * this.y + 
                           this.z * this.z + this.w * this.w;
                };
                
                def magnitude() -> float
                {
                    return math::basic::sqrt(this.magnitude_squared());
                };
                
                def normalize() -> vector4
                {
                    float mag = this.magnitude();
                    if (mag < math::constants::FLOAT_EPSILON)
                    {
                        throw("Cannot normalize zero vector4");
                    };
                    return vector4(this.x / mag, this.y / mag, 
                                 this.z / mag, this.w / mag);
                };
                
                def xyz() -> vector3
                {
                    return vector3(this.x, this.y, this.z);
                };
                
                def to_vector() -> vector
                {
                    vector result(4);
                    result.set(0, this.x);
                    result.set(1, this.y);
                    result.set(2, this.z);
                    result.set(3, this.w);
                    return result;
                };
            };
        };
        
        // ================================================================
        // VECTOR OPERATIONS
        // ================================================================
        
        namespace operations
        {
            // Vector creation utilities
            def create_vector(basic::i32 dimensions, float initial_value) -> basic::vector
            {
                basic::vector result(dimensions);
                result.fill(initial_value);
                return result;
            };
            
            def zeros(basic::i32 dimensions) -> basic::vector
            {
                return create_vector(dimensions, 0.0);
            };
            
            def ones(basic::i32 dimensions) -> basic::vector
            {
                return create_vector(dimensions, 1.0);
            };
            
            def unit_vector(basic::i32 dimensions, basic::i32 axis) -> basic::vector
            {
                if (axis >= dimensions or axis < 0)
                {
                    throw("Unit vector axis out of bounds");
                };
                
                basic::vector result = zeros(dimensions);
                result.set(axis, 1.0);
                return result;
            };
            
            def random_vector(basic::i32 dimensions, float min_val, float max_val) -> basic::vector
            {
                basic::vector result(dimensions);
                for (basic::i32 i = 0; i < dimensions; i++)
                {
                    result.set(i, math::random::rand_range(min_val, max_val));
                };
                return result;
            };
            
            def random_unit_vector(basic::i32 dimensions) -> basic::vector
            {
                basic::vector result(dimensions);
                
                // Use normal distribution for each component
                for (basic::i32 i = 0; i < dimensions; i++)
                {
                    result.set(i, math::random::rand_normal(0.0, 1.0));
                };
                
                return result.normalize();
            };
            
            // Vector arithmetic operations
            def add(basic::vector a, basic::vector b) -> basic::vector
            {
                return a.__add(b);
            };
            
            def subtract(basic::vector a, basic::vector b) -> basic::vector
            {
                return a.__sub(b);
            };
            
            def scalar_multiply(basic::vector v, float scalar) -> basic::vector
            {
                return v.__mul(scalar);
            };
            
            def element_wise_multiply(basic::vector a, basic::vector b) -> basic::vector
            {
                if (a.dimensions != b.dimensions)
                {
                    throw("Vector dimensions must match for element-wise multiplication");
                };
                
                basic::vector result(a.dimensions);
                for (basic::i32 i = 0; i < a.dimensions; i++)
                {
                    result.set(i, a.at(i) * b.at(i));
                };
                
                return result;
            };
            
            def element_wise_divide(basic::vector a, basic::vector b) -> basic::vector
            {
                if (a.dimensions != b.dimensions)
                {
                    throw("Vector dimensions must match for element-wise division");
                };
                
                basic::vector result(a.dimensions);
                for (basic::i32 i = 0; i < a.dimensions; i++)
                {
                    float denominator = b.at(i);
                    if (math::basic::abs(denominator) < math::constants::FLOAT_EPSILON)
                    {
                        throw("Division by zero in element-wise division");
                    };
                    result.set(i, a.at(i) / denominator);
                };
                
                return result;
            };
            
            // Vector products and operations
            def dot_product(basic::vector a, basic::vector b) -> float
            {
                return a.dot(b);
            };
            
            def cross_product_3d(basic::vector3 a, basic::vector3 b) -> basic::vector3
            {
                return a.cross(b);
            };
            
            def outer_product(basic::vector a, basic::vector b) -> basic::vector
            {
                // Returns flattened outer product as vector
                basic::vector result(a.dimensions * b.dimensions);
                for (basic::i32 i = 0; i < a.dimensions; i++)
                {
                    for (basic::i32 j = 0; j < b.dimensions; j++)
                    {
                        result.set(i * b.dimensions + j, a.at(i) * b.at(j));
                    };
                };
                return result;
            };
            
            // Distance metrics
            def euclidean_distance(basic::vector a, basic::vector b) -> float
            {
                return a.distance_to(b);
            };
            
            def manhattan_distance(basic::vector a, basic::vector b) -> float
            {
                if (a.dimensions != b.dimensions)
                {
                    throw("Vector dimensions must match for Manhattan distance");
                };
                
                float distance = 0.0;
                for (basic::i32 i = 0; i < a.dimensions; i++)
                {
                    distance += math::basic::abs(a.at(i) - b.at(i));
                };
                
                return distance;
            };
            
            def chebyshev_distance(basic::vector a, basic::vector b) -> float
            {
                if (a.dimensions != b.dimensions)
                {
                    throw("Vector dimensions must match for Chebyshev distance");
                };
                
                float max_distance = 0.0;
                for (basic::i32 i = 0; i < a.dimensions; i++)
                {
                    float component_distance = math::basic::abs(a.at(i) - b.at(i));
                    max_distance = math::basic::max(max_distance, component_distance);
                };
                
                return max_distance;
            };
            
            def minkowski_distance(basic::vector a, basic::vector b, float p) -> float
            {
                if (a.dimensions != b.dimensions)
                {
                    throw("Vector dimensions must match for Minkowski distance");
                };
                if (p <= 0.0)
                {
                    throw("Minkowski distance parameter p must be positive");
                };
                
                float sum = 0.0;
                for (basic::i32 i = 0; i < a.dimensions; i++)
                {
                    float diff = math::basic::abs(a.at(i) - b.at(i));
                    sum += math::exponential::pow(diff, p);
                };
                
                return math::exponential::pow(sum, 1.0 / p);
            };
            
            def cosine_similarity(basic::vector a, basic::vector b) -> float
            {
                float dot_prod = a.dot(b);
                float mag_product = a.magnitude() * b.magnitude();
                
                if (mag_product < math::constants::FLOAT_EPSILON)
                {
                    throw("Cannot compute cosine similarity with zero vector");
                };
                
                return dot_prod / mag_product;
            };
            
            def cosine_distance(basic::vector a, basic::vector b) -> float
            {
                return 1.0 - cosine_similarity(a, b);
            };
            
            // Vector norms
            def l1_norm(basic::vector v) -> float
            {
                float sum = 0.0;
                for (basic::i32 i = 0; i < v.dimensions; i++)
                {
                    sum += math::basic::abs(v.at(i));
                };
                return sum;
            };
            
            def l2_norm(basic::vector v) -> float
            {
                return v.magnitude();
            };
            
            def lp_norm(basic::vector v, float p) -> float
            {
                if (p <= 0.0)
                {
                    throw("Norm parameter p must be positive");
                };
                
                float sum = 0.0;
                for (basic::i32 i = 0; i < v.dimensions; i++)
                {
                    sum += math::exponential::pow(math::basic::abs(v.at(i)), p);
                };
                
                return math::exponential::pow(sum, 1.0 / p);
            };
            
            def infinity_norm(basic::vector v) -> float
            {
                return v.abs().component_max();
            };
            
            // Vector transformations
            def gram_schmidt_orthogonalize(basic::vector vectors[], basic::i32 count) -> void
            {
                if (count == 0) { return void; };
                
                // Normalize first vector
                vectors[0] = vectors[0].normalize();
                
                for (basic::i32 i = 1; i < count; i++)
                {
                    basic::vector v = vectors[i];
                    
                    // Subtract projections onto previous vectors
                    for (basic::i32 j = 0; j < i; j++)
                    {
                        basic::vector projection = v.project_onto(vectors[j]);
                        v -= projection;
                    };
                    
                    // Normalize result
                    vectors[i] = v.normalize();
                };
                
                return void;
            };
            
            def principal_component_analysis(basic::vector data[], basic::i32 count, basic::i32 num_components) -> basic::vector
            {
                if (count == 0 or num_components <= 0)
                {
                    throw("Invalid parameters for PCA");
                };
                
                basic::i32 dimensions = data[0].dimensions;
                
                // Compute mean vector
                basic::vector mean = zeros(dimensions);
                for (basic::i32 i = 0; i < count; i++)
                {
                    mean += data[i];
                };
                mean /= (float)count;
                
                // Center the data
                for (basic::i32 i = 0; i < count; i++)
                {
                    data[i] -= mean;
                };
                
                // For simplicity, return the mean vector
                // A full PCA implementation would compute eigenvalues of covariance matrix
                return mean;
            };
            
            // Vector utilities
            def centroid(basic::vector vectors[], basic::i32 count) -> basic::vector
            {
                if (count == 0)
                {
                    throw("Cannot compute centroid of empty vector set");
                };
                
                basic::vector result = zeros(vectors[0].dimensions);
                for (basic::i32 i = 0; i < count; i++)
                {
                    result += vectors[i];
                };
                
                return result / (float)count;
            };
            
            def variance(basic::vector vectors[], basic::i32 count) -> float
            {
                if (count <= 1)
                {
                    return 0.0;
                };
                
                basic::vector mean_vec = centroid(vectors, count);
                float variance = 0.0;
                
                for (basic::i32 i = 0; i < count; i++)
                {
                    float distance = vectors[i].distance_to(mean_vec);
                    variance += distance * distance;
                };
                
                return variance / (float)(count - 1);
            };
        };
        
        // ================================================================
        // SPECIALIZED VECTORS
        // ================================================================
        
        namespace specialized
        {
            // Sparse vector using coordinate format
            object sparse_vector
            {
                basic::i32* indices;
                float* values;
                basic::i32 nnz; // Number of non-zero elements
                basic::i32 capacity;
                basic::i32 dimensions;
                
                def __init(basic::i32 dim, basic::i32 max_nnz) -> this
                {
                    this.dimensions = dim;
                    this.nnz = 0;
                    this.capacity = max_nnz;
                    // TODO: Allocate arrays
                    return this;
                };
                
                def __exit() -> void
                {
                    if (this.indices is !void) { (void)this.indices; };
                    if (this.values is !void) { (void)this.values; };
                    return void;
                };
                
                def add_element(basic::i32 index, float value) -> void
                {
                    if (this.nnz >= this.capacity)
                    {
                        throw("Sparse vector capacity exceeded");
                    };
                    if (math::basic::abs(value) < math::constants::FLOAT_EPSILON)
                    {
                        return void; // Don't store zeros
                    };
                    
                    this.indices[this.nnz] = index;
                    this.values[this.nnz] = value;
                    this.nnz++;
                    return void;
                };
                
                def at(basic::i32 index) -> float
                {
                    for (basic::i32 i = 0; i < this.nnz; i++)
                    {
                        if (this.indices[i] == index)
                        {
                            return this.values[i];
                        };
                    };
                    return 0.0;
                };
                
                def to_vector() -> basic::vector
                {
                    basic::vector result(this.dimensions);
                    result.zero();
                    
                    for (basic::i32 i = 0; i < this.nnz; i++)
                    {
                        result.set(this.indices[i], this.values[i]);
                    };
                    
                    return result;
                };
                
                def dot(sparse_vector other) -> float
                {
                    float result = 0.0;
                    
                    basic::i32 i = 0, j = 0;
                    while (i < this.nnz and j < other.nnz)
                    {
                        if (this.indices[i] == other.indices[j])
                        {
                            result += this.values[i] * other.values[j];
                            i++;
                            j++;
                        }
                        else if (this.indices[i] < other.indices[j])
                        {
                            i++;
                        }
                        else
                        {
                            j++;
                        };
                    };
                    
                    return result;
                };
            };
            
            // Complex number as 2D vector
            object complex
            {
                float real, imag;
                
                def __init() -> this
                {
                    this.real = 0.0;
                    this.imag = 0.0;
                    return this;
                };
                
                def __init(float r, float i) -> this
                {
                    this.real = r;
                    this.imag = i;
                    return this;
                };
                
                def __exit() -> void
                {
                    return void;
                };
                
                def __add(complex other) -> complex
                {
                    return complex(this.real + other.real, this.imag + other.imag);
                };
                
                def __sub(complex other) -> complex
                {
                    return complex(this.real - other.real, this.imag - other.imag);
                };
                
                def __mul(complex other) -> complex
                {
                    float new_real = this.real * other.real - this.imag * other.imag;
                    float new_imag = this.real * other.imag + this.imag * other.real;
                    return complex(new_real, new_imag);
                };
                
                def __div(complex other) -> complex
                {
                    float denominator = other.real * other.real + other.imag * other.imag;
                    if (math::basic::abs(denominator) < math::constants::FLOAT_EPSILON)
                    {
                        throw("Division by zero complex number");
                    };
                    
                    float new_real = (this.real * other.real + this.imag * other.imag) / denominator;
                    float new_imag = (this.imag * other.real - this.real * other.imag) / denominator;
                    return complex(new_real, new_imag);
                };
                
                def __iadd(complex other) -> this
                {
                    this.real += other.real;
                    this.imag += other.imag;
                    return this;
                };
                
                def __isub(complex other) -> this
                {
                    this.real -= other.real;
                    this.imag -= other.imag;
                    return this;
                };
                
                def __imul(complex other) -> this
                {
                    float new_real = this.real * other.real - this.imag * other.imag;
                    float new_imag = this.real * other.imag + this.imag * other.real;
                    this.real = new_real;
                    this.imag = new_imag;
                    return this;
                };
                
                def __idiv(complex other) -> this
                {
                    float denominator = other.real * other.real + other.imag * other.imag;
                    if (math::basic::abs(denominator) < math::constants::FLOAT_EPSILON)
                    {
                        throw("Division by zero complex number");
                    };
                    
                    float new_real = (this.real * other.real + this.imag * other.imag) / denominator;
                    float new_imag = (this.imag * other.real - this.real * other.imag) / denominator;
                    this.real = new_real;
                    this.imag = new_imag;
                    return this;
                };
                
                def magnitude() -> float
                {
                    return math::basic::sqrt(this.real * this.real + this.imag * this.imag);
                };
                
                def phase() -> float
                {
                    return math::trigonometric::atan2(this.imag, this.real);
                };
                
                def conjugate() -> complex
                {
                    return complex(this.real, -this.imag);
                };
                
                def to_vector2() -> basic::vector2
                {
                    return basic::vector2(this.real, this.imag);
                };
            };
            
            // Quaternion for 3D rotations
            object quaternion
            {
                float w, x, y, z;
                
                def __init() -> this
                {
                    this.w = 1.0;
                    this.x = 0.0;
                    this.y = 0.0;
                    this.z = 0.0;
                    return this;
                };
                
                def __init(float w_val, float x_val, float y_val, float z_val) -> this
                {
                    this.w = w_val;
                    this.x = x_val;
                    this.y = y_val;
                    this.z = z_val;
                    return this;
                };
                
                def __exit() -> void
                {
                    return void;
                };
                
                def __mul(quaternion other) -> quaternion
                {
                    float new_w = this.w * other.w - this.x * other.x - this.y * other.y - this.z * other.z;
                    float new_x = this.w * other.x + this.x * other.w + this.y * other.z - this.z * other.y;
                    float new_y = this.w * other.y - this.x * other.z + this.y * other.w + this.z * other.x;
                    float new_z = this.w * other.z + this.x * other.y - this.y * other.x + this.z * other.w;
                    return quaternion(new_w, new_x, new_y, new_z);
                };
                
                def __imul(quaternion other) -> this
                {
                    float new_w = this.w * other.w - this.x * other.x - this.y * other.y - this.z * other.z;
                    float new_x = this.w * other.x + this.x * other.w + this.y * other.z - this.z * other.y;
                    float new_y = this.w * other.y - this.x * other.z + this.y * other.w + this.z * other.x;
                    float new_z = this.w * other.z + this.x * other.y - this.y * other.x + this.z * other.w;
                    this.w = new_w;
                    this.x = new_x;
                    this.y = new_y;
                    this.z = new_z;
                    return this;
                };
                
                def magnitude() -> float
                {
                    return math::basic::sqrt(this.w * this.w + this.x * this.x + 
                                           this.y * this.y + this.z * this.z);
                };
                
                def normalize() -> quaternion
                {
                    float mag = this.magnitude();
                    if (mag < math::constants::FLOAT_EPSILON)
                    {
                        throw("Cannot normalize zero quaternion");
                    };
                    return quaternion(this.w / mag, this.x / mag, this.y / mag, this.z / mag);
                };
                
                def conjugate() -> quaternion
                {
                    return quaternion(this.w, -this.x, -this.y, -this.z);
                };
                
                def inverse() -> quaternion
                {
                    float mag_sq = this.w * this.w + this.x * this.x + this.y * this.y + this.z * this.z;
                    if (mag_sq < math::constants::FLOAT_EPSILON)
                    {
                        throw("Cannot invert zero quaternion");
                    };
                    
                    quaternion conj = this.conjugate();
                    return quaternion(conj.w / mag_sq, conj.x / mag_sq, conj.y / mag_sq, conj.z / mag_sq);
                };
                
                def from_axis_angle(basic::vector3 axis, float angle) -> quaternion
                {
                    float half_angle = angle / 2.0;
                    float sin_half = math::trigonometric::sin(half_angle);
                    basic::vector3 normalized_axis = axis.normalize();
                    
                    return quaternion(math::trigonometric::cos(half_angle),
                                    normalized_axis.x * sin_half,
                                    normalized_axis.y * sin_half,
                                    normalized_axis.z * sin_half);
                };
                
                def rotate_vector(basic::vector3 v) -> basic::vector3
                {
                    quaternion v_quat(0.0, v.x, v.y, v.z);
                    quaternion result = this.__mul(v_quat).__mul(this.conjugate());
                    return basic::vector3(result.x, result.y, result.z);
                };
                
                def to_vector4() -> basic::vector4
                {
                    return basic::vector4(this.w, this.x, this.y, this.z);
                };
            };
            
            // Homogeneous coordinates for transformations
            object homogeneous_vector3
            {
                float x, y, z, w;
                
                def __init(float x_val, float y_val, float z_val) -> this
                {
                    this.x = x_val;
                    this.y = y_val;
                    this.z = z_val;
                    this.w = 1.0;
                    return this;
                };
                
                def __init(float x_val, float y_val, float z_val, float w_val) -> this
                {
                    this.x = x_val;
                    this.y = y_val;
                    this.z = z_val;
                    this.w = w_val;
                    return this;
                };
                
                def __exit() -> void
                {
                    return void;
                };
                
                def normalize_homogeneous() -> homogeneous_vector3
                {
                    if (math::basic::abs(this.w) < math::constants::FLOAT_EPSILON)
                    {
                        throw("Cannot normalize homogeneous vector with zero w component");
                    };
                    return homogeneous_vector3(this.x / this.w, this.y / this.w, this.z / this.w, 1.0);
                };
                
                def to_vector3() -> basic::vector3
                {
                    homogeneous_vector3 normalized = this.normalize_homogeneous();
                    return basic::vector3(normalized.x, normalized.y, normalized.z);
                };
                
                def to_vector4() -> basic::vector4
                {
                    return basic::vector4(this.x, this.y, this.z, this.w);
                };
                
                def is_point() -> bool
                {
                    return math::basic::abs(this.w - 1.0) < math::constants::FLOAT_EPSILON;
                };
                
                def is_direction() -> bool
                {
                    return math::basic::abs(this.w) < math::constants::FLOAT_EPSILON;
                };
            };
        };
        
        // ================================================================
        // VECTOR UTILITIES
        // ================================================================
        
        namespace utilities
        {
            // Vector sorting and searching
            def find_closest_vector(basic::vector target, basic::vector candidates[], basic::i32 count) -> basic::i32
            {
                if (count == 0)
                {
                    throw("Cannot find closest vector in empty set");
                };
                
                basic::i32 closest_index = 0;
                float min_distance = target.distance_to(candidates[0]);
                
                for (basic::i32 i = 1; i < count; i++)
                {
                    float distance = target.distance_to(candidates[i]);
                    if (distance < min_distance)
                    {
                        min_distance = distance;
                        closest_index = i;
                    };
                };
                
                return closest_index;
            };
            
            def convex_hull_2d(basic::vector2 points[], basic::i32 count) -> basic::vector2
            {
                // Simplified convex hull - returns centroid for now
                if (count == 0)
                {
                    throw("Cannot compute convex hull of empty point set");
                };
                
                basic::vector2 centroid();
                for (basic::i32 i = 0; i < count; i++)
                {
                    centroid += points[i];
                };
                
                return centroid / (float)count;
            };
            
            // Vector interpolation and curves
            def bezier_curve_2d(basic::vector2 p0, basic::vector2 p1, basic::vector2 p2, basic::vector2 p3, float t) -> basic::vector2
            {
                float u = 1.0 - t;
                float uu = u * u;
                float uuu = uu * u;
                float tt = t * t;
                float ttt = tt * t;
                
                basic::vector2 result = p0 * uuu;
                result += p1 * (3.0 * uu * t);
                result += p2 * (3.0 * u * tt);
                result += p3 * ttt;
                
                return result;
            };
            
            def catmull_rom_spline_2d(basic::vector2 p0, basic::vector2 p1, basic::vector2 p2, basic::vector2 p3, float t) -> basic::vector2
            {
                float tt = t * t;
                float ttt = tt * t;
                
                basic::vector2 result = p0 * (-0.5 * ttt + tt - 0.5 * t);
                result += p1 * (1.5 * ttt - 2.5 * tt + 1.0);
                result += p2 * (-1.5 * ttt + 2.0 * tt + 0.5 * t);
                result += p3 * (0.5 * ttt - 0.5 * tt);
                
                return result;
            };
            
            // Vector analysis
            def is_orthogonal_set(basic::vector vectors[], basic::i32 count, float tolerance) -> bool
            {
                for (basic::i32 i = 0; i < count; i++)
                {
                    for (basic::i32 j = i + 1; j < count; j++)
                    {
                        float dot_product = vectors[i].dot(vectors[j]);
                        if (math::basic::abs(dot_product) > tolerance)
                        {
                            return false;
                        };
                    };
                };
                return true;
            };
            
            def is_orthonormal_set(basic::vector vectors[], basic::i32 count, float tolerance) -> bool
            {
                if (!is_orthogonal_set(vectors, count, tolerance))
                {
                    return false;
                };
                
                for (basic::i32 i = 0; i < count; i++)
                {
                    if (!vectors[i].is_normalized(tolerance))
                    {
                        return false;
                    };
                };
                
                return true;
            };
            
            def linear_independence_rank(basic::vector vectors[], basic::i32 count, float tolerance) -> basic::i32
            {
                if (count == 0) { return 0; };
                
                // Simple rank computation using Gram-Schmidt process
                basic::vector orthogonal[count];
                basic::i32 rank = 0;
                
                for (basic::i32 i = 0; i < count; i++)
                {
                    basic::vector v = vectors[i];
                    
                    // Subtract projections onto previous orthogonal vectors
                    for (basic::i32 j = 0; j < rank; j++)
                    {
                        basic::vector projection = v.project_onto(orthogonal[j]);
                        v = v.__sub(projection);
                    };
                    
                    // Check if resulting vector is significant
                    if (v.magnitude() > tolerance)
                    {
                        orthogonal[rank] = v.normalize();
                        rank++;
                    };
                };
                
                return rank;
            };
            
            // Vector transformations
            def reflect_across_plane(basic::vector v, basic::vector plane_normal) -> basic::vector
            {
                basic::vector normal = plane_normal.normalize();
                return v.reflect(normal);
            };
            
            def rotate_2d(basic::vector2 v, float angle) -> basic::vector2
            {
                return v.rotate(angle);
            };
            
            def rotate_3d_axis_angle(basic::vector3 v, basic::vector3 axis, float angle) -> basic::vector3
            {
                specialized::quaternion q = specialized::quaternion().from_axis_angle(axis, angle);
                return q.rotate_vector(v);
            };
            
            // Vector field operations
            def gradient_2d(float (*scalar_field)(float, float), basic::vector2 point, float h) -> basic::vector2
            {
                float dx = ((*scalar_field)(point.x + h, point.y) - (*scalar_field)(point.x - h, point.y)) / (2.0 * h);
                float dy = ((*scalar_field)(point.x, point.y + h) - (*scalar_field)(point.x, point.y - h)) / (2.0 * h);
                return basic::vector2(dx, dy);
            };
            
            def divergence_2d(basic::vector2 (*vector_field)(float, float), basic::vector2 point, float h) -> float
            {
                basic::vector2 fx_plus = (*vector_field)(point.x + h, point.y);
                basic::vector2 fx_minus = (*vector_field)(point.x - h, point.y);
                basic::vector2 fy_plus = (*vector_field)(point.x, point.y + h);
                basic::vector2 fy_minus = (*vector_field)(point.x, point.y - h);
                
                float dFx_dx = (fx_plus.x - fx_minus.x) / (2.0 * h);
                float dFy_dy = (fy_plus.y - fy_minus.y) / (2.0 * h);
                
                return dFx_dx + dFy_dy;
            };
            
            def curl_2d(basic::vector2 (*vector_field)(float, float), basic::vector2 point, float h) -> float
            {
                basic::vector2 fx_plus = (*vector_field)(point.x + h, point.y);
                basic::vector2 fx_minus = (*vector_field)(point.x - h, point.y);
                basic::vector2 fy_plus = (*vector_field)(point.x, point.y + h);
                basic::vector2 fy_minus = (*vector_field)(point.x, point.y - h);
                
                float dFy_dx = (fy_plus.y - fy_minus.y) / (2.0 * h);
                float dFx_dy = (fx_plus.x - fx_minus.x) / (2.0 * h);
                
                return dFy_dx - dFx_dy;
            };
            
            // Clustering and classification helpers
            def k_means_centroid_update(basic::vector points[], basic::i32 point_count, 
                                      basic::i32 assignments[], basic::i32 cluster_id) -> basic::vector
            {
                if (point_count == 0)
                {
                    throw("Cannot update centroid with no points");
                };
                
                basic::vector centroid = operations::zeros(points[0].dimensions);
                basic::i32 cluster_point_count = 0;
                
                for (basic::i32 i = 0; i < point_count; i++)
                {
                    if (assignments[i] == cluster_id)
                    {
                        centroid += points[i];
                        cluster_point_count++;
                    };
                };
                
                if (cluster_point_count == 0)
                {
                    return centroid; // Return zero vector for empty cluster
                };
                
                return centroid / (float)cluster_point_count;
            };
            
            // Vector format conversion
            def vector_to_array(basic::vector v, float output[]) -> void
            {
                for (basic::i32 i = 0; i < v.dimensions; i++)
                {
                    output[i] = v.at(i);
                };
                return void;
            };
            
            def array_to_vector(float input[], basic::i32 size) -> basic::vector
            {
                basic::vector result(size);
                for (basic::i32 i = 0; i < size; i++)
                {
                    result.set(i, input[i]);
                };
                return result;
            };
            
            def vector2_to_polar(basic::vector2 cartesian) -> basic::vector2
            {
                float r = cartesian.magnitude();
                float theta = cartesian.angle();
                return basic::vector2(r, theta);
            };
            
            def polar_to_vector2(basic::vector2 polar) -> basic::vector2
            {
                float x = polar.x * math::trigonometric::cos(polar.y);
                float y = polar.x * math::trigonometric::sin(polar.y);
                return basic::vector2(x, y);
            };
            
            def vector3_to_spherical(basic::vector3 cartesian) -> basic::vector3
            {
                float r = cartesian.magnitude();
                float theta = math::trigonometric::atan2(cartesian.y, cartesian.x);
                float phi = math::trigonometric::acos(cartesian.z / r);
                return basic::vector3(r, theta, phi);
            };
            
            def spherical_to_vector3(basic::vector3 spherical) -> basic::vector3
            {
                float r = spherical.x;
                float theta = spherical.y;
                float phi = spherical.z;
                
                float x = r * math::trigonometric::sin(phi) * math::trigonometric::cos(theta);
                float y = r * math::trigonometric::sin(phi) * math::trigonometric::sin(theta);
                float z = r * math::trigonometric::cos(phi);
                
                return basic::vector3(x, y, z);
            };
        };
        
        // ================================================================
        // COMMON VECTOR CONSTANTS
        // ================================================================
        
        namespace constants
        {
            // Common 2D vectors
            basic::vector2 VECTOR2_ZERO(0.0, 0.0);
            basic::vector2 VECTOR2_ONE(1.0, 1.0);
            basic::vector2 VECTOR2_UP(0.0, 1.0);
            basic::vector2 VECTOR2_DOWN(0.0, -1.0);
            basic::vector2 VECTOR2_LEFT(-1.0, 0.0);
            basic::vector2 VECTOR2_RIGHT(1.0, 0.0);
            
            // Common 3D vectors
            basic::vector3 VECTOR3_ZERO(0.0, 0.0, 0.0);
            basic::vector3 VECTOR3_ONE(1.0, 1.0, 1.0);
            basic::vector3 VECTOR3_UP(0.0, 1.0, 0.0);
            basic::vector3 VECTOR3_DOWN(0.0, -1.0, 0.0);
            basic::vector3 VECTOR3_LEFT(-1.0, 0.0, 0.0);
            basic::vector3 VECTOR3_RIGHT(1.0, 0.0, 0.0);
            basic::vector3 VECTOR3_FORWARD(0.0, 0.0, 1.0);
            basic::vector3 VECTOR3_BACKWARD(0.0, 0.0, -1.0);
            
            // Common quaternions
            specialized::quaternion QUATERNION_IDENTITY(1.0, 0.0, 0.0, 0.0);
        };
    };
};