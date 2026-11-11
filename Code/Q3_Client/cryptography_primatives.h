#ifndef CRYPTOGRAPHY_PRIMATIVES_H
#define CRYPTOGRAPHY_PRIMATIVES_H

#include <openssl/evp.h>
#include "SecretData.h"

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
    SecretData<32> generateX25519SharedSecret(EVP_PKEY* peerPublicKey, EVP_PKEY* privateKey);

    EVP_PKEY* generateEd25519Key();
    std::array<uint8_t, 64> signWithEd25519(EVP_PKEY* key, const std::vector<uint8_t>& plaintext);
    bool checkSignWithEd25519(EVP_PKEY* key, const std::array<uint8_t,64> signature, const std::vector<uint8_t>& plaintext);

    SecretData<32> generateHKDFKey(const std::span<const std::uint8_t>& sharedSecret, const std::vector<uint8_t>& salt, const std::vector<uint8_t>& info);
    SecretData<32> generateHKDFKey(const std::span<const std::uint8_t>& sharedSecret);

    EVP_PKEY* generateML_KEM_1024Key();

}

#endif // CRYPTOGRAPHY_PRIMATIVES_H