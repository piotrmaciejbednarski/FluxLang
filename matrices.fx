import "math.fx";

namespace std {
    class Matrices {
        // Dimension type union
        union MatrixDim {
            int{32} dim32;
            int{64} dim64;
        };

        // Matrix structure with dimension unions
        struct Matrix {
            MatrixDim rows;
            MatrixDim cols;
            bool is64bit;      // Flag to indicate which union member to use
            float{64}[] data;  // Row-major order
        };

        object MatrixOps {
            // Create a new 32-bit matrix
            Matrix create32(int{32} rows, int{32} cols) {
                Matrix m;
                m.rows.dim32 = rows;
                m.cols.dim32 = cols;
                m.is64bit = false;
                m.data = memalloc(float{64}[rows * cols]);
                return m;
            };

            // Create a new 64-bit matrix
            Matrix create64(int{64} rows, int{64} cols) {
                Matrix m;
                m.rows.dim64 = rows;
                m.cols.dim64 = cols;
                m.is64bit = true;
                m.data = memalloc(float{64}[rows * cols]);
                return m;
            };

            // Create an identity matrix (32-bit)
            Matrix identity32(int{32} size) {
                Matrix m = create32(size, size);
                
                for (int{32} i = 0; i < size; i++) {
                    for (int{32} j = 0; j < size; j++) {
                        m.data[i * size + j] = (i == j) ? 1.0 : 0.0;
                    };
                };
                
                return m;
            };

            // Create an identity matrix (64-bit)
            Matrix identity64(int{64} size) {
                Matrix m = create64(size, size);
                
                for (int{64} i = 0; i < size; i++) {
                    for (int{64} j = 0; j < size; j++) {
                        m.data[i * size + j] = (i == j) ? 1.0 : 0.0;
                    };
                };
                
                return m;
            };

            // Get element at position (i,j)
            float{64} get(Matrix m, int{64} i, int{64} j) {
                int{64} rows = m.is64bit ? m.rows.dim64 : int{64}:m.rows.dim32;
                int{64} cols = m.is64bit ? m.cols.dim64 : int{64}:m.cols.dim32;

                if (i < 0 || i >= rows || j < 0 || j >= cols) {
                    throw("Matrix index out of bounds");
                };
                return m.data[i * cols + j];
            };

            // Set element at position (i,j)
            void set(Matrix m, int{64} i, int{64} j, float{64} value) {
                int{64} rows = m.is64bit ? m.rows.dim64 : int{64}:m.rows.dim32;
                int{64} cols = m.is64bit ? m.cols.dim64 : int{64}:m.cols.dim32;

                if (i < 0 || i >= rows || j < 0 || j >= cols) {
                    throw("Matrix index out of bounds");
                };
                m.data[i * cols + j] = value;
            };

            // Matrix addition
            Matrix add(Matrix a, Matrix b) {
                if (a.is64bit != b.is64bit) {
                    throw("Matrix bit widths do not match");
                };

                int{64} aRows = a.is64bit ? a.rows.dim64 : int{64}:a.rows.dim32;
                int{64} aCols = a.is64bit ? a.cols.dim64 : int{64}:a.cols.dim32;
                int{64} bRows = b.is64bit ? b.rows.dim64 : int{64}:b.rows.dim32;
                int{64} bCols = b.is64bit ? b.cols.dim64 : int{64}:b.cols.dim32;

                if (aRows != bRows || aCols != bCols) {
                    throw("Matrix dimensions do not match");
                };
                
                Matrix result = a.is64bit ? create64(aRows, aCols) : create32(int{32}:aRows, int{32}:aCols);
                
                for (int{64} i = 0; i < aRows; i++) {
                    for (int{64} j = 0; j < aCols; j++) {
                        int{64} idx = i * aCols + j;
                        result.data[idx] = a.data[idx] + b.data[idx];
                    };
                };
                
                return result;
            };

            // Matrix subtraction
            Matrix subtract(Matrix a, Matrix b) {
                if (a.is64bit != b.is64bit) {
                    throw("Matrix bit widths do not match");
                };

                int{64} aRows = a.is64bit ? a.rows.dim64 : int{64}:a.rows.dim32;
                int{64} aCols = a.is64bit ? a.cols.dim64 : int{64}:a.cols.dim32;
                int{64} bRows = b.is64bit ? b.rows.dim64 : int{64}:b.rows.dim32;
                int{64} bCols = b.is64bit ? b.cols.dim64 : int{64}:b.cols.dim32;

                if (aRows != bRows || aCols != bCols) {
                    throw("Matrix dimensions do not match");
                };
                
                Matrix result = a.is64bit ? create64(aRows, aCols) : create32(int{32}:aRows, int{32}:aCols);
                
                for (int{64} i = 0; i < aRows; i++) {
                    for (int{64} j = 0; j < aCols; j++) {
                        int{64} idx = i * aCols + j;
                        result.data[idx] = a.data[idx] - b.data[idx];
                    };
                };
                
                return result;
            };

            // Matrix multiplication
            Matrix multiply(Matrix a, Matrix b) {
                if (a.is64bit != b.is64bit) {
                    throw("Matrix bit widths do not match");
                };

                int{64} aRows = a.is64bit ? a.rows.dim64 : int{64}:a.rows.dim32;
                int{64} aCols = a.is64bit ? a.cols.dim64 : int{64}:a.cols.dim32;
                int{64} bRows = b.is64bit ? b.rows.dim64 : int{64}:b.rows.dim32;
                int{64} bCols = b.is64bit ? b.cols.dim64 : int{64}:b.cols.dim32;

                if (aCols != bRows) {
                    throw("Matrix dimensions are not compatible for multiplication");
                };
                
                Matrix result = a.is64bit ? create64(aRows, bCols) : create32(int{32}:aRows, int{32}:bCols);
                
                for (int{64} i = 0; i < aRows; i++) {
                    for (int{64} j = 0; j < bCols; j++) {
                        float{64} sum = 0.0;
                        for (int{64} k = 0; k < aCols; k++) {
                            sum = sum + a.data[i * aCols + k] * b.data[k * bCols + j];
                        };
                        result.data[i * bCols + j] = sum;
                    };
                };
                
                return result;
            };

            // Matrix transpose
            Matrix transpose(Matrix m) {
                int{64} rows = m.is64bit ? m.rows.dim64 : int{64}:m.rows.dim32;
                int{64} cols = m.is64bit ? m.cols.dim64 : int{64}:m.cols.dim32;
                
                Matrix result = m.is64bit ? create64(cols, rows) : create32(int{32}:cols, int{32}:rows);
                
                for (int{64} i = 0; i < rows; i++) {
                    for (int{64} j = 0; j < cols; j++) {
                        result.data[j * rows + i] = m.data[i * cols + j];
                    };
                };
                
                return result;
            };

            // Matrix scalar multiplication
            Matrix scalarMultiply(Matrix m, float{64} scalar) {
                int{64} rows = m.is64bit ? m.rows.dim64 : int{64}:m.rows.dim32;
                int{64} cols = m.is64bit ? m.cols.dim64 : int{64}:m.cols.dim32;
                
                Matrix result = m.is64bit ? create64(rows, cols) : create32(int{32}:rows, int{32}:cols);
                
                for (int{64} i = 0; i < rows * cols; i++) {
                    result.data[i] = m.data[i] * scalar;
                };
                
                return result;
            };

            // Calculate determinant (for square matrices)
            float{64} determinant(Matrix m) {
                int{64} rows = m.is64bit ? m.rows.dim64 : int{64}:m.rows.dim32;
                int{64} cols = m.is64bit ? m.cols.dim64 : int{64}:m.cols.dim32;

                if (rows != cols) {
                    throw("Determinant only defined for square matrices");
                };

                if (rows == 1) {
                    return m.data[0];
                };

                if (rows == 2) {
                    return m.data[0] * m.data[3] - m.data[1] * m.data[2];
                };

                float{64} det = 0.0;
                int{64} sign = 1;

                for (int{64} j = 0; j < cols; j++) {
                    // Create submatrix
                    Matrix sub = m.is64bit ? create64(rows - 1, cols - 1) : create32(int{32}:(rows - 1), int{32}:(cols - 1));
                    int{64} subi = 0;
                    
                    for (int{64} i = 1; i < rows; i++) {
                        int{64} subj = 0;
                        for (int{64} k = 0; k < cols; k++) {
                            if (k != j) {
                                sub.data[subi * (cols - 1) + subj] = m.data[i * cols + k];
                                subj = subj + 1;
                            };
                        };
                        subi = subi + 1;
                    };
                    
                    det = det + sign * m.data[j] * determinant(sub);
                    sign = -sign;
                };

                return det;
            };
        };
    };
};
