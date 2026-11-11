#ifndef CRYPTOGRAPHY_PRIMATIVES_H
#define CRYPTOGRAPHY_PRIMATIVES_H

#include <openssl/evp.h>

#include <vector>
#include <array>
#include <string>

namespace Q3Crypto
{
    struct curvePrekeyWithIdentifiers
    {
        EVP_PKEY* key;
        int identifier;
    };

    //this is replacing XEdDSA
    struct identityKey
    {
        EVP_PKEY* X25519_key;

    };

    std::vector<uint8_t> hexToBytes(const std::string& hex);
    std::array<uint8_t, 64> sha512(const std::vector<uint8_t>& plaintext);

    EVP_PKEY* generateX25519Key();
    std::vector<curvePrekeyWithIdentifiers> generatePreKeysX25519(int count, int identiferStart);

    EVP_PKEY* generateEd25519Key();
    std::array<uint8_t, 64> signWithEd25519(EVP_PKEY* key, const std::vector<uint8_t> plaintext);
}

#endif // CRYPTOGRAPHY_PRIMATIVES_H