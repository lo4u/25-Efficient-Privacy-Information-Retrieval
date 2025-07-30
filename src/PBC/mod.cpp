#include "mod.h"

// Utility function that computes a hash of the key and mods it by the given modulus
size_t hash_and_mod(
    size_t id, 
    size_t nonce, 
    const std::vector<unsigned char> data, 
    size_t modulus
) {
    // Use SHA-256 for hashing
    SHA256_CTX sha256;
    unsigned char hash[SHA256_DIGEST_LENGTH];

    // Initialize SHA-256 context
    SHA256_Init(&sha256);

    // Hash the custom string (id and nonce)
    std::stringstream ss;
    ss << id << nonce;
    SHA256_Update(&sha256, ss.str().c_str(), ss.str().size());

    // Hash the data
    SHA256_Update(&sha256, data.data(), data.size());

    // Finalize the hash
    SHA256_Final(hash, &sha256);

    // Convert hash to a big integer
    boost::multiprecision::cpp_int int_value;
    for (unsigned int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
        int_value = (int_value << 8) | hash[i];
    }

    // Perform modulo operation
    return static_cast<size_t>(int_value % modulus);
}

