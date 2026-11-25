#ifndef CRYPTOGRAPHY_PRIMATIVES_H
#define CRYPTOGRAPHY_PRIMATIVES_H

#include <openssl/evp.h>
#include "SecretData.h"
#include <QString>
#include <QJsonObject>
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

    inline constexpr std::size_t PrivateKeySizeX25519    = 32;
    inline constexpr std::size_t PrivateKeySizeEd25519   = 32;
    inline constexpr std::size_t PrivateKeySizeML_KEM_1024 = 64;

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

    struct prekeyWithIdentifierBytesX25519
    {
        std::array<std::uint8_t, 1 + PublicKeySizeX25519> key;
        int identifier;
    };

    struct prekeyWithIdentifierML_KEM_1024
    {
        EVP_PKEY_ML_KEM_1024_ptr key;
        int identifier;
    };

    struct prekeyWithIdentifierBytesML_KEM_1024
    {
        std::array<std::uint8_t, 1 + PublicKeySizeML_KEM_1024> key;
        int identifier;
    };

    struct prekeyWithIdentifierSignedBytesX25519
    {
        std::array<std::uint8_t, 1 + PublicKeySizeX25519> key;
        int identifier;
        std::array<uint8_t, 64> signature;
    };

    struct prekeyWithIdentifierSignedBytesML_KEM_1024
    {
        std::array<std::uint8_t, 1 + PublicKeySizeML_KEM_1024> key;
        int identifier;
        std::array<uint8_t, 64> signature;
    };

    struct identityKeyPublicBytes
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
        std::array<uint8_t, 12> nonce;
        std::vector<uint8_t> ciphertext;
        std::array<uint8_t, 16> tag;
    };

    std::vector<uint8_t> hexToBytes(const std::string& hex);
    std::array<uint8_t, 64> sha512(const std::vector<uint8_t>& plaintext);

    EVP_PKEY_X25519_ptr generateX25519Key();
    std::vector<prekeyWithIdentifierX25519> generatePreKeysX25519(int count, int& identiferStart);
    SecretData<32> generateX25519SharedSecret(const EVP_PKEY_X25519_ptr& privateKey, const EVP_PKEY_X25519_ptr& peerPublicKey);

    EVP_PKEY_Ed25519_ptr generateEd25519Key();
    std::array<uint8_t, 64> signWithEd25519(EVP_PKEY_Ed25519_ptr& key, const std::span<const std::uint8_t>& plaintext);
    bool checkSignWithEd25519(const EVP_PKEY_Ed25519_ptr& key, const std::array<uint8_t,64> signature, const std::span<const std::uint8_t>& plaintext);

    SecretData<32> generateHKDFKey(const std::span<const std::uint8_t>& sharedSecret, const std::vector<uint8_t>& salt, const std::vector<uint8_t>& info);
    SecretData<32> generateHKDFKey(const std::span<const std::uint8_t>& sharedSecret);

    EVP_PKEY_ML_KEM_1024_ptr generateML_KEM_1024Key();
    std::vector<prekeyWithIdentifierML_KEM_1024> generatePreKeysML_KEM_1024(int count, int& identiferStart);
    ML_KEM_1024Result encapsulateWithML_KEM_1024(const EVP_PKEY_ML_KEM_1024_ptr& peerPublicKey);
    SecretData <32> decapsulateWithML_KEM_1024(const EVP_PKEY_ML_KEM_1024_ptr& privateKey, const std::array<uint8_t, 1568>& ciphertext);

    AES_256Result encryptWithAES_256_GCM(const SecretData <32>& key, const std::vector<uint8_t>& plaintext, const std::vector<uint8_t>& associatedData);
    std::vector<uint8_t> decryptWithAES_256_GCM(const SecretData <32>& key, const AES_256Result& encrypted, const std::vector<uint8_t>& associatedData);

    std::vector<uint8_t> getPublicKeyBytes(EVP_PKEY* key);

    std::array<std::uint8_t, PublicKeySizeX25519 + 1> encodePublicX25519(const EVP_PKEY_X25519_ptr& key);
    std::array<std::uint8_t, PublicKeySizeEd25519 + 1> encodePublicEd25519(const EVP_PKEY_Ed25519_ptr& key);
    std::array<std::uint8_t, PublicKeySizeML_KEM_1024 + 1> encodePublicML_KEM_1024(const EVP_PKEY_ML_KEM_1024_ptr& key);
    EVP_PKEY_X25519_ptr decodePublicX25519(std::array<std::uint8_t, PublicKeySizeX25519 + 1> encoded);
    EVP_PKEY_Ed25519_ptr decodePublicEd25519(std::array<std::uint8_t, PublicKeySizeEd25519 + 1> encoded);
    EVP_PKEY_ML_KEM_1024_ptr decodePublicML_KEM_1024(std::array<std::uint8_t, PublicKeySizeML_KEM_1024 + 1> encoded);

    SecretData<PrivateKeySizeX25519 + 1> encodePrivateX25519(const EVP_PKEY_X25519_ptr& key);
    SecretData<PrivateKeySizeEd25519 + 1> encodePrivateEd25519(const EVP_PKEY_Ed25519_ptr& key);
    SecretData<PrivateKeySizeML_KEM_1024 + 1> encodePrivateML_KEM_1024(const EVP_PKEY_ML_KEM_1024_ptr& key);

    EVP_PKEY_X25519_ptr decodePrivateX25519(SecretData<PrivateKeySizeX25519 + 1> encoded);
    EVP_PKEY_Ed25519_ptr decodePrivateEd25519(SecretData<PrivateKeySizeEd25519 + 1> encoded);
    EVP_PKEY_ML_KEM_1024_ptr decodePrivateML_KEM_1024(const SecretData<PrivateKeySizeML_KEM_1024 + 1>& encoded);

    QString bytesToBase64(const std::uint8_t* data, std::size_t size);
    QJsonObject curvePrekeyToJson(const prekeyWithIdentifierSignedBytesX25519& prekey);
    QJsonObject pqkemPrekeyToJson(const prekeyWithIdentifierSignedBytesML_KEM_1024& prekey);

    void base64FieldToBytes(const QJsonObject& json, const char* field, std::uint8_t* out, std::size_t expectedSize);
    prekeyWithIdentifierSignedBytesX25519 curvePrekeyFromJson(const QJsonObject& json);
    prekeyWithIdentifierSignedBytesML_KEM_1024 pqkemPrekeyFromJson(const QJsonObject& json);


}

#endif // CRYPTOGRAPHY_PRIMATIVES_H