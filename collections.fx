namespace std {
    class Collections {
        // Dynamic List implementation
        object List {
            // List state structure
            struct ListState {
                int{32} capacity;
                int{32} length;
                object[] elements;
            };

            // Constructor
            ListState init() {
                ListState state;
                state.capacity = 16;  // Initial capacity
                state.length = 0;
                state.elements = memalloc(object[state.capacity]);
                return state;
            };

            // Add element to end of list
            void append(ListState state, object element) {
                if (state.length == state.capacity) {
                    // Double capacity when full
                    state.capacity = state.capacity * 2;
                    object[] newElements = memalloc(object[state.capacity]);
                    
                    // Copy old elements
                    for (int{32} i = 0; i < state.length; i++) {
                        newElements[i] = state.elements[i];
                    };
                    
                    state.elements = newElements;
                };
                
                state.elements[state.length] = element;
                state.length = state.length + 1;
            };

            // Insert element at index
            void insert(ListState state, int{32} index, object element) {
                if (index < 0 || index > state.length) {
                    throw("Index out of bounds");
                };
                
                if (state.length == state.capacity) {
                    state.capacity = state.capacity * 2;
                    object[] newElements = memalloc(object[state.capacity]);
                    
                    // Copy elements before index
                    for (int{32} i = 0; i < index; i++) {
                        newElements[i] = state.elements[i];
                    };
                    
                    // Insert new element
                    newElements[index] = element;
                    
                    // Copy elements after index
                    for (int{32} i = index; i < state.length; i++) {
                        newElements[i + 1] = state.elements[i];
                    };
                    
                    state.elements = newElements;
                } else {
                    // Shift elements right
                    for (int{32} i = state.length; i > index; i--) {
                        state.elements[i] = state.elements[i - 1];
                    };
                    
                    // Insert new element
                    state.elements[index] = element;
                };
                
                state.length = state.length + 1;
            };

            // Remove element at index
            void remove(ListState state, int{32} index) {
                if (index < 0 || index >= state.length) {
                    throw("Index out of bounds");
                };
                
                // Shift elements left
                for (int{32} i = index; i < state.length - 1; i++) {
                    state.elements[i] = state.elements[i + 1];
                };
                
                state.length = state.length - 1;
            };

            // Get element at index
            object get(ListState state, int{32} index) {
                if (index < 0 || index >= state.length) {
                    throw("Index out of bounds");
                };
                return state.elements[index];
            };

            // Set element at index
            void set(ListState state, int{32} index, object element) {
                if (index < 0 || index >= state.length) {
                    throw("Index out of bounds");
                };
                state.elements[index] = element;
            };

            // Get list length
            int{32} length(ListState state) {
                return state.length;
            };

            // Clear list
            void clear(ListState state) {
                state.length = 0;
            };
        };

        // Binary data operations
        object Bytes {
            // Convert integer types to bytes
            int{8}[] fromInt32(int{32} value) {
                int{8}[] bytes = [
                    int{8}:(value >> 24),
                    int{8}:((value >> 16) & 0xFF),
                    int{8}:((value >> 8) & 0xFF),
                    int{8}:(value & 0xFF)
                ];
                return bytes;
            };

            int{8}[] fromInt64(int{64} value) {
                int{8}[] bytes = [
                    int{8}:(value >> 56),
                    int{8}:((value >> 48) & 0xFF),
                    int{8}:((value >> 40) & 0xFF),
                    int{8}:((value >> 32) & 0xFF),
                    int{8}:((value >> 24) & 0xFF),
                    int{8}:((value >> 16) & 0xFF),
                    int{8}:((value >> 8) & 0xFF),
                    int{8}:(value & 0xFF)
                ];
                return bytes;
            };

            // Convert float types to bytes
            int{8}[] fromFloat32(float{32} value) {
                int{32} bits = int{32}:value;
                return fromInt32(bits);
            };

            int{8}[] fromFloat64(float{64} value) {
                int{64} bits = int{64}:value;
                return fromInt64(bits);
            };

            // Convert string to bytes (UTF-8 encoding)
            int{8}[] fromString(string value) {
                int{8}[] bytes = [];
                for (int{32} i = 0; i < length(value); i++) {
                    int{32} codepoint = int{32}:value[i];
                    
                    if (codepoint < 0x80) {
                        bytes = bytes + [int{8}:codepoint];
                    } else if (codepoint < 0x800) {
                        bytes = bytes + [
                            int{8}:(0xC0 | (codepoint >> 6)),
                            int{8}:(0x80 | (codepoint & 0x3F))
                        ];
                    } else if (codepoint < 0x10000) {
                        bytes = bytes + [
                            int{8}:(0xE0 | (codepoint >> 12)),
                            int{8}:(0x80 | ((codepoint >> 6) & 0x3F)),
                            int{8}:(0x80 | (codepoint & 0x3F))
                        ];
                    } else {
                        bytes = bytes + [
                            int{8}:(0xF0 | (codepoint >> 18)),
                            int{8}:(0x80 | ((codepoint >> 12) & 0x3F)),
                            int{8}:(0x80 | ((codepoint >> 6) & 0x3F)),
                            int{8}:(0x80 | (codepoint & 0x3F))
                        ];
                    };
                };
                return bytes;
            };

            // Convert from bytes back to types
            int{32} toInt32(int{8}[] bytes) {
                if (length(bytes) != 4) {
                    throw("Invalid byte array length for int32");
                };
                
                return (int{32}:bytes[0] << 24) |
                       (int{32}:bytes[1] << 16) |
                       (int{32}:bytes[2] << 8) |
                       int{32}:bytes[3];
            };

            int{64} toInt64(int{8}[] bytes) {
                if (length(bytes) != 8) {
                    throw("Invalid byte array length for int64");
                };
                
                return (int{64}:bytes[0] << 56) |
                       (int{64}:bytes[1] << 48) |
                       (int{64}:bytes[2] << 40) |
                       (int{64}:bytes[3] << 32) |
                       (int{64}:bytes[4] << 24) |
                       (int{64}:bytes[5] << 16) |
                       (int{64}:bytes[6] << 8) |
                       int{64}:bytes[7];
            };

            float{32} toFloat32(int{8}[] bytes) {
                return float{32}:toInt32(bytes);
            };

            float{64} toFloat64(int{8}[] bytes) {
                return float{64}:toInt64(bytes);
            };

            string toString(int{8}[] bytes) {
                string result = "";
                int{32} i = 0;
                
                while (i < length(bytes)) {
                    int{8} b = bytes[i];
                    
                    if ((b & 0x80) == 0) {
                        result = result + string:int{32}:b;
                        i = i + 1;
                    } else if ((b & 0xE0) == 0xC0) {
                        int{32} codepoint = ((int{32}:b & 0x1F) << 6) |
                                          (int{32}:bytes[i + 1] & 0x3F);
                        result = result + string:codepoint;
                        i = i + 2;
                    } else if ((b & 0xF0) == 0xE0) {
                        int{32} codepoint = ((int{32}:b & 0x0F) << 12) |
                                          ((int{32}:bytes[i + 1] & 0x3F) << 6) |
                                          (int{32}:bytes[i + 2] & 0x3F);
                        result = result + string:codepoint;
                        i = i + 3;
                    } else if ((b & 0xF8) == 0xF0) {
                        int{32} codepoint = ((int{32}:b & 0x07) << 18) |
                                          ((int{32}:bytes[i + 1] & 0x3F) << 12) |
                                          ((int{32}:bytes[i + 2] & 0x3F) << 6) |
                                          (int{32}:bytes[i + 3] & 0x3F);
                        result = result + string:codepoint;
                        i = i + 4;
                    };
                };
                
                return result;
            };

            // Utility functions
            string toHex(int{8}[] bytes) {
                string result = "";
                string hexChars = "0123456789ABCDEF";
                
                for (int{32} i = 0; i < length(bytes); i++) {
                    int{8} b = bytes[i];
                    result = result + hexChars[int{32}:(b >> 4)] + hexChars[int{32}:(b & 0x0F)];
                };
                
                return result;
            };

            int{8}[] fromHex(string hex) {
                if (length(hex) % 2 != 0) {
                    throw("Invalid hex string length");
                };
                
                int{8}[] bytes = [];
                
                for (int{32} i = 0; i < length(hex); i = i + 2) {
                    string byte = hex[i:i+2];
                    int{32} value = 0;
                    
                    for (int{32} j = 0; j < 2; j++) {
                        value = value << 4;
                        if (byte[j] >= '0' && byte[j] <= '9') {
                            value = value | (int{32}:byte[j] - int{32}:'0');
                        } else if (byte[j] >= 'A' && byte[j] <= 'F') {
                            value = value | (int{32}:byte[j] - int{32}:'A' + 10);
                        } else if (byte[j] >= 'a' && byte[j] <= 'f') {
                            value = value | (int{32}:byte[j] - int{32}:'a' + 10);
                        } else {
                            throw("Invalid hex character");
                        };
                    };
                    
                    bytes = bytes + [int{8}:value];
                };
                
                return bytes;
            };
        };
    };
};
