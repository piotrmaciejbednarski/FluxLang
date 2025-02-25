import "math.fx";

namespace std {
    class Tensor {
        // Tensor structure using pointers
        struct TensorState {
            int{32}[] shape;     // Dimensions of the tensor
            int{32} rank;         // Number of dimensions
            float{64} *data;      // Pointer to data storage
            int{32} *refCount;    // Reference counting for memory management
        };

        object TensorOps {
            // Create a tensor with specified shape
            TensorState create(int{32}[] shape, float{64} initialValue = 0.0) {
                // Calculate total number of elements
                int{32} totalElements = 1;
                for (int{32} i = 0; i < length(shape); i++) {
                    totalElements = totalElements * shape[i];
                };
                
                // Allocate tensor
                TensorState tensor;
                tensor.shape = memalloc(int{32}[length(shape)]);
                
                // Copy shape
                for (int{32} i = 0; i < length(shape); i++) {
                    tensor.shape[i] = shape[i];
                };
                
                tensor.rank = length(shape);
                tensor.data = memalloc(float{64}[totalElements]);
                
                // Allocate reference counter
                tensor.refCount = memalloc(int{32}[1]);
                tensor.refCount[0] = 1;
                
                // Initialize with given value
                for (int{32} i = 0; i < totalElements; i++) {
                    tensor.data[i] = initialValue;
                };
                
                return tensor;
            };

            // Create a tensor of zeros
            TensorState zeros(int{32}[] shape) {
                return create(shape, 0.0);
            };

            // Create a tensor of ones
            TensorState ones(int{32}[] shape) {
                return create(shape, 1.0);
            };

            // Create an identity matrix (for 2D tensors)
            TensorState eye(int{32} size) {
                int{32}[] shape = [size, size];
                TensorState tensor = create(shape);
                
                for (int{32} i = 0; i < size; i++) {
                    tensor.data[i * size + i] = 1.0;
                };
                
                return tensor;
            };

            // Create a tensor with random values
            TensorState random(int{32}[] shape, 
                               float{64} min = 0.0, 
                               float{64} max = 1.0) {
                TensorState tensor = create(shape);
                
                int{32} totalElements = 1;
                for (int{32} i = 0; i < length(shape); i++) {
                    totalElements = totalElements * shape[i];
                };
                
                for (int{32} i = 0; i < totalElements; i++) {
                    // Simple pseudo-random generation
                    float{64} r = float{64}:(i * 1664525 + 1013904223) / float{64}:totalElements;
                    tensor.data[i] = min + r * (max - min);
                };
                
                return tensor;
            };

            // Create a view (reference) of an existing tensor
            TensorState view(TensorState original) {
                TensorState view = original;
                view.refCount[0]++;
                return view;
            };

            // Deallocate tensor memory
            void free(TensorState tensor) {
                tensor.refCount[0]--;
                
                if (tensor.refCount[0] <= 0) {
                    // Actually free memory
                    delete(tensor.data);
                    delete(tensor.shape);
                    delete(tensor.refCount);
                };
            };

            // Perform a deep copy
            TensorState copy(TensorState tensor) {
                TensorState newTensor = create(tensor.shape);
                
                // Copy data
                int{32} totalElements = 1;
                for (int{32} i = 0; i < tensor.rank; i++) {
                    totalElements = totalElements * tensor.shape[i];
                };
                
                for (int{32} i = 0; i < totalElements; i++) {
                    newTensor.data[i] = tensor.data[i];
                };
                
                return newTensor;
            };

            // Get element at specific indices
            float{64} get(TensorState tensor, int{32}[] indices) {
                if (length(indices) != tensor.rank) {
                    throw("Index dimensions do not match tensor rank");
                };
                
                // Calculate linear index
                int{32} linearIndex = 0;
                int{32} stride = 1;
                
                for (int{32} i = tensor.rank - 1; i >= 0; i--) {
                    if (indices[i] < 0 || indices[i] >= tensor.shape[i]) {
                        throw("Index out of bounds");
                    };
                    
                    linearIndex = linearIndex + indices[i] * stride;
                    stride = stride * tensor.shape[i];
                };
                
                return tensor.data[linearIndex];
            };

            // Set element at specific indices
            void set(TensorState tensor, int{32}[] indices, float{64} value) {
                if (length(indices) != tensor.rank) {
                    throw("Index dimensions do not match tensor rank");
                };
                
                // Calculate linear index
                int{32} linearIndex = 0;
                int{32} stride = 1;
                
                for (int{32} i = tensor.rank - 1; i >= 0; i--) {
                    if (indices[i] < 0 || indices[i] >= tensor.shape[i]) {
                        throw("Index out of bounds");
                    };
                    
                    linearIndex = linearIndex + indices[i] * stride;
                    stride = stride * tensor.shape[i];
                };
                
                tensor.data[linearIndex] = value;
            };

            // Element-wise operations using Math library
            TensorState add(TensorState a, TensorState b) {
                // Validate tensor compatibility
                if (a.rank != b.rank) {
                    throw("Tensor ranks do not match");
                };
                
                for (int{32} i = 0; i < a.rank; i++) {
                    if (a.shape[i] != b.shape[i]) {
                        throw("Tensor dimensions do not match");
                    };
                };
                
                // Create result tensor
                TensorState result = create(a.shape);
                
                // Perform element-wise addition
                int{32} totalElements = 1;
                for (int{32} i = 0; i < a.rank; i++) {
                    totalElements = totalElements * a.shape[i];
                };
                
                for (int{32} i = 0; i < totalElements; i++) {
                    result.data[i] = a.data[i] + b.data[i];
                };
                
                return result;
            };

            // Element-wise subtraction
            TensorState subtract(TensorState a, TensorState b) {
                // Validate tensor compatibility
                if (a.rank != b.rank) {
                    throw("Tensor ranks do not match");
                };
                
                for (int{32} i = 0; i < a.rank; i++) {
                    if (a.shape[i] != b.shape[i]) {
                        throw("Tensor dimensions do not match");
                    };
                };
                
                // Create result tensor
                TensorState result = create(a.shape);
                
                // Perform element-wise subtraction
                int{32} totalElements = 1;
                for (int{32} i = 0; i < a.rank; i++) {
                    totalElements = totalElements * a.shape[i];
                };
                
                for (int{32} i = 0; i < totalElements; i++) {
                    result.data[i] = a.data[i] - b.data[i];
                };
                
                return result;
            };

            // Element-wise multiplication
            TensorState multiply(TensorState a, TensorState b) {
                // Validate tensor compatibility
                if (a.rank != b.rank) {
                    throw("Tensor ranks do not match");
                };
                
                for (int{32} i = 0; i < a.rank; i++) {
                    if (a.shape[i] != b.shape[i]) {
                        throw("Tensor dimensions do not match");
                    };
                };
                
                // Create result tensor
                TensorState result = create(a.shape);
                
                // Perform element-wise multiplication
                int{32} totalElements = 1;
                for (int{32} i = 0; i < a.rank; i++) {
                    totalElements = totalElements * a.shape[i];
                };
                
                for (int{32} i = 0; i < totalElements; i++) {
                    result.data[i] = a.data[i] * b.data[i];
                };
                
                return result;
            };

            // Scalar multiplication
            TensorState scalarMultiply(TensorState tensor, float{64} scalar) {
                // Create result tensor
                TensorState result = create(tensor.shape);
                
                // Perform scalar multiplication
                int{32} totalElements = 1;
                for (int{32} i = 0; i < tensor.rank; i++) {
                    totalElements = totalElements * tensor.shape[i];
                };
                
                for (int{32} i = 0; i < totalElements; i++) {
                    result.data[i] = tensor.data[i] * scalar;
                };
                
                return result;
            };

            // Matrix multiplication for 2D tensors
            TensorState matmul(TensorState a, TensorState b) {
                // Check tensor compatibility for matrix multiplication
                if (a.rank != 2 || b.rank != 2) {
                    throw("Matrix multiplication requires 2D tensors");
                };
                
                if (a.shape[1] != b.shape[0]) {
                    throw("Incompatible matrix dimensions for multiplication");
                };
                
                // Create result tensor
                int{32}[] resultShape = [a.shape[0], b.shape[1]];
                TensorState result = create(resultShape);
                
                // Perform matrix multiplication
                for (int{32} i = 0; i < a.shape[0]; i++) {
                    for (int{32} j = 0; j < b.shape[1]; j++) {
                        float{64} sum = 0.0;
                        for (int{32} k = 0; k < a.shape[1]; k++) {
                            sum = sum + a.data[i * a.shape[1] + k] * b.data[k * b.shape[1] + j];
                        };
                        result.data[i * result.shape[1] + j] = sum;
                    };
                };
                
                return result;
            };

            // Transpose for 2D tensors
            TensorState transpose(TensorState tensor) {
                if (tensor.rank != 2) {
                    throw("Transpose only supported for 2D tensors");
                };
                
                // Create result tensor with swapped dimensions
                int{32}[] transposedShape = [tensor.shape[1], tensor.shape[0]];
                TensorState result = create(transposedShape);
                
                // Perform transpose
                for (int{32} i = 0; i < tensor.shape[0]; i++) {
                    for (int{32} j = 0; j < tensor.shape[1]; j++) {
                        result.data[j * result.shape[1] + i] = tensor.data[i * tensor.shape[1] + j];
                    };
                };
                
                return result;
            };

            // Element-wise mathematical functions from Math library
            TensorState sin(TensorState tensor) {
                TensorState result = create(tensor.shape);
                
                int{32} totalElements = 1;
                for (int{32} i = 0; i < tensor.rank; i++) {
                    totalElements = totalElements * tensor.shape[i];
                };
                
                for (int{32} i = 0; i < totalElements; i++) {
                    result.data[i] = Math.Trigonometry.sin(tensor.data[i]);
                };
                
                return result;
            };

            TensorState cos(TensorState tensor) {
                TensorState result = create(tensor.shape);
                
                int{32} totalElements = 1;
                for (int{32} i = 0; i < tensor.rank; i++) {
                    totalElements = totalElements * tensor.shape[i];
                };
                
                for (int{32} i = 0; i < totalElements; i++) {
                    result.data[i] = Math.Trigonometry.cos(tensor.data[i]);
                };
                
                return result;
            };

            TensorState exp(TensorState tensor) {
                TensorState result = create(tensor.shape);
                
                int{32} totalElements = 1;
                for (int{32} i = 0; i < tensor.rank; i++) {
                    totalElements = totalElements * tensor.shape[i];
                };
                
                for (int{32} i = 0; i < totalElements; i++) {
                    result.data[i] = Math.Exponential.exp(tensor.data[i]);
                };
                
                return result;
            };

            TensorState log(TensorState tensor) {
                TensorState result = create(tensor.shape);
                
                int{32} totalElements = 1;
                for (int{32} i = 0; i < tensor.rank; i++) {
                    totalElements = totalElements * tensor.shape[i];
                };
                
                for (int{32} i = 0; i < totalElements; i++) {
                    result.data[i] = Math.Exponential.log(tensor.data[i]);
                };
                
                return result;
            };

            // Reduction operations
            float{64} sum(TensorState tensor) {
                float{64} total = 0.0;
                
                int{32} totalElements = 1;
                for (int{32} i = 0; i < tensor.rank; i++) {
                    totalElements = totalElements * tensor.shape[i];
                };
                
                for (int{32} i = 0; i < totalElements; i++) {
                    total = total + tensor.data[i];
                };
                
                return total;
            };

            float{64} mean(TensorState tensor) {
                float{64} total = sum(tensor);
                
                int{32} totalElements = 1;
                for (int{32} i = 0; i < tensor.rank; i++) {
                    totalElements = totalElements * tensor.shape[i];
                };
                
                return total / float{64}:totalElements;
            };
        };
    };
};
