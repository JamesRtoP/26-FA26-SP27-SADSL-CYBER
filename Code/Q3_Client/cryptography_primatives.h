#ifndef CRYPTOGRAPHY_PRIMATIVES_H
#define CRYPTOGRAPHY_PRIMATIVES_H

#include <openssl/evp.h>
#include "SecretData.h"

#include <vector>
#include <array>
#include <string>

namespace Q3Crypto
{
    struct prekeyWithIdentifier
    {
        EVP_PKEY* key;
        int identifier;
    };

    //this is replacing XEdDSA
    struct identityKey
    {
        EVP_PKEY* X25519_key;
        EVP_PKEY* Ed25519_key;
    };

    struct ML_KEM_1024Result
    {
        std::array<uint8_t, 1568> ciphertext;
        SecretData<32> sharedSecret{};
    };

    struct AES_256Result
    {
        std::array<uint8_t, 12> nonce{};
        std::vector<uint8_t> ciphertext;
        std::array<uint8_t, 16> tag{};
    };

    std::vector<uint8_t> hexToBytes(const std::string& hex);
    std::array<uint8_t, 64> sha512(const std::vector<uint8_t>& plaintext);

    EVP_PKEY* generateX25519Key();
    std::vector<prekeyWithIdentifier> generatePreKeysX25519(int count, int& identiferStart);
    SecretData<32> generateX25519SharedSecret(EVP_PKEY* peerPublicKey, EVP_PKEY* privateKey);

    EVP_PKEY* generateEd25519Key();
    std::array<uint8_t, 64> signWithEd25519(EVP_PKEY* key, const std::vector<uint8_t>& plaintext);
    bool checkSignWithEd25519(EVP_PKEY* key, const std::array<uint8_t,64> signature, const std::vector<uint8_t>& plaintext);

    SecretData<32> generateHKDFKey(const std::span<const std::uint8_t>& sharedSecret, const std::vector<uint8_t>& salt, const std::vector<uint8_t>& info);
    SecretData<32> generateHKDFKey(const std::span<const std::uint8_t>& sharedSecret);

    EVP_PKEY* generateML_KEM_1024Key();
    std::vector<prekeyWithIdentifier> generatePreKeysML_KEM_1024(int count, int& identiferStart);
    ML_KEM_1024Result encapsulateWithML_KEM_1024(EVP_PKEY* peerPublicKey);
    SecretData <32> decapsulateWithML_KEM_1024(EVP_PKEY* privateKey, const std::array<uint8_t, 1568>& ciphertext);
    AES_256Result encryptWithAES256GCM(const SecretData <32>& key, const std::vector<uint8_t>& plaintext, const std::vector<uint8_t>& associatedData);


}

#endif // CRYPTOGRAPHY_PRIMATIVES_H