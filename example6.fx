namespace crypto {
    // SHA-256 context object
    object SHA256 {
        def __init() -> void;
        def update(data[] input) -> void;
        def final() -> data[32];
        
        // Utility functions
        static def hash(data[] input) -> data[32];
        static def hash_string(string str) -> data[32];
    };
};

object crypto::SHA256 {
    // Initial hash values (first 32 bits of fractional parts of square roots of first 8 primes)
    const data[32] H0 = {0x6a09e667};
    const data[32] H1 = {0xbb67ae85};
    const data[32] H2 = {0x3c6ef372};
    const data[32] H3 = {0xa54ff53a};
    const data[32] H4 = {0x510e527f};
    const data[32] H5 = {0x9b05688c};
    const data[32] H6 = {0x1f83d9ab};
    const data[32] H7 = {0x5be0cd19};

    // Round constants (first 32 bits of fractional parts of cube roots of first 64 primes)
    const data[32][64] K = [
        0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
        0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
        0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
        0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
        0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
        0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
        0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
        0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
    ];

    // Internal state
    data[32] state[8];
    data[64] buffer;
    int buffer_len;
    int total_len;

    def __init() -> void {
        this.reset();
    };

    def reset() -> void {
        this.state[0] = H0;
        this.state[1] = H1;
        this.state[2] = H2;
        this.state[3] = H3;
        this.state[4] = H4;
        this.state[5] = H5;
        this.state[6] = H6;
        this.state[7] = H7;
        this.buffer_len = 0;
        this.total_len = 0;
    };

    def update(data[] input) -> void {
        this.total_len += input.len() * 8; // Count bits
        
        for (byte in input) {
            this.buffer[this.buffer_len] = byte;
            this.buffer_len += 1;
            
            if (this.buffer_len == 64) {
                this._transform();
                this.buffer_len = 0;
            }
        }
    };

    def final() -> data[32] {
        // Pad the message
        int pad_len = (this.buffer_len < 56) ? (56 - this.buffer_len) : (120 - this.buffer_len);
        data[] padding = [0x80] + [0x00] * (pad_len - 1);
        
        // Append length (big-endian)
        data[8] length_bytes;
        for (i in 0..8) {
            length_bytes[7 - i] = (this.total_len >> (i * 8)) & 0xff;
        }
        
        this.update(padding);
        this.update(length_bytes);
        
        // Final hash
        data[32] hash;
        for (i in 0..8) {
            for (j in 0..4) {
                hash[i * 4 + j] = (this.state[i] >> (24 - j * 8)) & 0xff;
            }
        }
        
        this.reset();
        return hash;
    };

    // Internal transformation function
    def _transform() -> void {
        data[32] w[64];
        
        // Prepare message schedule
        for (i in 0..16) {
            w[i] = (this.buffer[i * 4] << 24) |
                   (this.buffer[i * 4 + 1] << 16) |
                   (this.buffer[i * 4 + 2] << 8) |
                   (this.buffer[i * 4 + 3]);
        }
        
        for (i in 16..64) {
            data[32] s0 = _right_rotate(w[i-15], 7) ^ _right_rotate(w[i-15], 18) ^ (w[i-15] >> 3);
            data[32] s1 = _right_rotate(w[i-2], 17) ^ _right_rotate(w[i-2], 19) ^ (w[i-2] >> 10);
            w[i] = w[i-16] + s0 + w[i-7] + s1;
        }
        
        // Initialize working variables
        data[32] a = this.state[0];
        data[32] b = this.state[1];
        data[32] c = this.state[2];
        data[32] d = this.state[3];
        data[32] e = this.state[4];
        data[32] f = this.state[5];
        data[32] g = this.state[6];
        data[32] h = this.state[7];
        
        // Compression function
        for (i in 0..64) {
            data[32] S1 = _right_rotate(e, 6) ^ _right_rotate(e, 11) ^ _right_rotate(e, 25);
            data[32] ch = (e & f) ^ ((~e) & g);
            data[32] temp1 = h + S1 + ch + K[i] + w[i];
            data[32] S0 = _right_rotate(a, 2) ^ _right_rotate(a, 13) ^ _right_rotate(a, 22);
            data[32] maj = (a & b) ^ (a & c) ^ (b & c);
            data[32] temp2 = S0 + maj;
            
            h = g;
            g = f;
            f = e;
            e = d + temp1;
            d = c;
            c = b;
            b = a;
            a = temp1 + temp2;
        }
        
        // Update state
        this.state[0] += a;
        this.state[1] += b;
        this.state[2] += c;
        this.state[3] += d;
        this.state[4] += e;
        this.state[5] += f;
        this.state[6] += g;
        this.state[7] += h;
    };

    // Helper functions
    static def _right_rotate(data[32] value, int shift) -> data[32] {
        return (value >> shift) | (value << (32 - shift));
    };

    // Static utility methods
    static def hash(data[] input) -> data[32] {
        SHA256{} ctx;
        ctx.update(input);
        return ctx.final();
    };

    static def hash_string(string str) -> data[32] {
        return SHA256.hash((data[])str);
    };
};

def main() -> int {
    // Example usage
    string message = "Hello, Flux!";
    data[32] hash = SHA256.hash_string(message);
    
    print("Message: " + message);
    print("SHA-256 Hash:");
    
    // Print hash as hex string
    string hex_chars = "0123456789abcdef";
    string hex_str = "";
    for (byte in hash) {
        hex_str += hex_chars[(byte >> 4) & 0xf];
        hex_str += hex_chars[byte & 0xf];
    }
    
    print(hex_str);
    return 0;
};