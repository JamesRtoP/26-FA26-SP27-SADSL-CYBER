#ifndef CRYPTOGRAPHY_PRIMATIVES_H
#define CRYPTOGRAPHY_PRIMATIVES_H

#include <openssl/evp.h>
#include "SecretData.h"
//#include "EncodeAndDecode.h"

#include <vector>
#include <array>
#include <string>
#include <memory>

namespace Q3Crypto
{

    inline constexpr std::size_t PublicKeySizeX25519    = 32;
    inline constexpr std::size_t PublicKeySizeEd25519   = 32;
    inline constexpr std::size_t PublicKeySizeML_KEM_1024 = 1568;

    struct EVP_PKEYDeleterX25519 {
        void operator()(EVP_PKEY* key) const noexcept { EVP_PKEY_free(key); }
    };

    struct EVP_PKEYDeleterML_KEM_1024 {
        void operator()(EVP_PKEY* key) const noexcept { EVP_PKEY_free(key); }
    };

    struct EVP_PKEYDeleterEd25519 {
        void operator()(EVP_PKEY* key) const noexcept { EVP_PKEY_free(key); }
    };

    using EVP_PKEY_X25519_ptr = std::unique_ptr<EVP_PKEY, EVP_PKEYDeleterX25519>;

    using EVP_PKEY_ML_KEM_1024_ptr = std::unique_ptr<EVP_PKEY, EVP_PKEYDeleterML_KEM_1024>;

    using EVP_PKEY_Ed25519_ptr = std::unique_ptr<EVP_PKEY, EVP_PKEYDeleterEd25519>;

    struct prekeyWithIdentifierX25519
    {
        EVP_PKEY_X25519_ptr key;
        int identifier;
    };

    struct prekeyWithIdentifierML_KEM_1024
    {
        EVP_PKEY_ML_KEM_1024_ptr key;
        int identifier;
    };

    struct prekeyBytesWithIdentifierSignedX25519
    {
        std::array<std::uint8_t, 1 + PublicKeySizeX25519> key;
        int identifier;
        std::array<uint8_t, 64> signature;
    };

    struct prekeyBytesWithIdentifierSignedML_KEM_1024
    {
        std::array<std::uint8_t, 1 + PublicKeySizeML_KEM_1024> key;
        int identifier;
        std::array<uint8_t, 64> signature;
    };

    struct identityKeyPairBytes
    {
        std::array<std::uint8_t, 1 + PublicKeySizeX25519> X25519_key;
        std::array<std::uint8_t, 1 + PublicKeySizeEd25519> Ed25519_key;
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

    EVP_PKEY_X25519_ptr generateX25519Key();
    std::vector<prekeyWithIdentifierX25519> generatePreKeysX25519(int count, int& identiferStart);
    SecretData<32> generateX25519SharedSecret(EVP_PKEY_X25519_ptr peerPublicKey, EVP_PKEY_X25519_ptr privateKey);

    EVP_PKEY_Ed25519_ptr generateEd25519Key();
    std::array<uint8_t, 64> signWithEd25519(EVP_PKEY_Ed25519_ptr& key, const std::span<const std::uint8_t>& plaintext);
    bool checkSignWithEd25519(EVP_PKEY_Ed25519_ptr key, const std::array<uint8_t,64> signature, const std::vector<uint8_t>& plaintext);

    SecretData<32> generateHKDFKey(const std::span<const std::uint8_t>& sharedSecret, const std::vector<uint8_t>& salt, const std::vector<uint8_t>& info);
    SecretData<32> generateHKDFKey(const std::span<const std::uint8_t>& sharedSecret);

    EVP_PKEY_ML_KEM_1024_ptr generateML_KEM_1024Key();
    std::vector<prekeyWithIdentifierML_KEM_1024> generatePreKeysML_KEM_1024(int count, int& identiferStart);
    ML_KEM_1024Result encapsulateWithML_KEM_1024(EVP_PKEY_ML_KEM_1024_ptr peerPublicKey);
    SecretData <32> decapsulateWithML_KEM_1024(EVP_PKEY_ML_KEM_1024_ptr privateKey, const std::array<uint8_t, 1568>& ciphertext);

    AES_256Result encryptWithAES_256_GCM(const SecretData <32>& key, const std::vector<uint8_t>& plaintext, const std::vector<uint8_t>& associatedData);
    std::vector<uint8_t> decryptWithAES_256_GCM(const SecretData <32>& key, const AES_256Result& encrypted, const std::vector<uint8_t>& associatedData);

    std::vector<uint8_t> getPublicKeyBytes(EVP_PKEY* key);

    std::array<std::uint8_t, PublicKeySizeX25519 + 1> encodeX25519(const EVP_PKEY_X25519_ptr& key);
    std::array<std::uint8_t, PublicKeySizeEd25519 + 1> encodeEd25519(const EVP_PKEY_Ed25519_ptr& key);
    std::array<std::uint8_t, PublicKeySizeML_KEM_1024 + 1> encodeML_KEM_1024(const EVP_PKEY_ML_KEM_1024_ptr& key);
    EVP_PKEY_X25519_ptr decodeX25519(std::array<std::uint8_t, PublicKeySizeX25519 + 1> encoded);
    EVP_PKEY_Ed25519_ptr decodeEd25519(std::array<std::uint8_t, PublicKeySizeEd25519 + 1> encoded);
    EVP_PKEY_ML_KEM_1024_ptr decodeML_KEM_1024(std::array<std::uint8_t, PublicKeySizeML_KEM_1024 + 1> encoded);

}

#endif // CRYPTOGRAPHY_PRIMATIVES_H