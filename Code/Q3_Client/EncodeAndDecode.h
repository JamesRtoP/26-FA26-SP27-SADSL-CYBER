#ifndef ENCODEANDDECODE_H
#define ENCODEANDDECODE_H

#include <stdexcept>
#include <string>
#include <openssl/evp.h>
#include <array>
#include <span>


namespace
{

    enum class KeyType : std::uint8_t {
        X25519    = 0,
        Ed25519   = 14,
        MLKEM1024 = 255,
    };

    // Not returning secret because encoded keys are public and shared.
    template <std::size_t N>
    std::array<std::uint8_t, N + 1> encodeWithPrefix(KeyType type, const EVP_PKEY* key, const char* keyAlgorithm)
    {
        if (key == nullptr)
            throw std::invalid_argument(std::string("encode: null ") + keyAlgorithm + " key");
        if (!EVP_PKEY_is_a(key, keyAlgorithm))
            throw std::invalid_argument(std::string("encode: key is not ") + keyAlgorithm);

        std::array<std::uint8_t, N + 1> encoded;
        encoded.data()[0] = static_cast<std::uint8_t>(type);

        std::size_t length = N;
        if (EVP_PKEY_get_raw_public_key(key, encoded.data() + 1, &length) != 1 || length != N)
            throw std::runtime_error(std::string("encode: failed to export ") + keyAlgorithm
                                     + " public key");
        return encoded;
    }

    template <std::size_t N>
    EVP_PKEY* decodeWithPrefix(KeyType expected, std::span<const std::uint8_t> encoded, const char* keyAlgorithmName)
    {
        const std::string name(keyAlgorithmName);

        if (encoded.empty())
            throw std::runtime_error("decode " + name + ": encoded key is empty");
        if (encoded[0] != static_cast<std::uint8_t>(expected))
            throw std::runtime_error("decode " + name + ": unrecognised key type byte "
                              + std::to_string(encoded[0]));
        if (encoded.size() != N + 1)
            throw std::runtime_error("decode " + name + ": expected " + std::to_string(N + 1)
                              + " bytes, got " + std::to_string(encoded.size()));

        EVP_PKEY* key(
            EVP_PKEY_new_raw_public_key_ex(nullptr, keyAlgorithmName, nullptr, encoded.data() + 1, N));
        if (!key)
            throw std::runtime_error("decode " + name + ": OpenSSL rejected the key");
        return key;
    }
}

namespace Q3Crypto
{

    inline constexpr std::size_t kX25519PublicKeySize    = 32;
    inline constexpr std::size_t kEd25519PublicKeySize   = 32;
    inline constexpr std::size_t kMLKEM1024PublicKeySize = 1568;

    inline std::array<std::uint8_t, 1 + kX25519PublicKeySize> encodeEC(const EVP_PKEY* key)
    {
        return encodeWithPrefix<kX25519PublicKeySize>(KeyType::X25519, key, "X25519");
    }

    inline std::array<std::uint8_t, 1 + kEd25519PublicKeySize> encodeSigningKey(const EVP_PKEY* key)
    {
        return encodeWithPrefix<kEd25519PublicKeySize>(KeyType::Ed25519, key, "ED25519");
    }

    inline std::array<std::uint8_t, 1 + kMLKEM1024PublicKeySize> encodeKEM(const EVP_PKEY* key)
    {
        return encodeWithPrefix<kMLKEM1024PublicKeySize>(KeyType::MLKEM1024, key, "ML-KEM-1024");
    }

    inline EVP_PKEY* decodeEC(std::span<const std::uint8_t> encoded)
    {
        return decodeWithPrefix<kX25519PublicKeySize>(KeyType::X25519, encoded, "X25519");
    }

    inline EVP_PKEY* decodeSigningKey(std::span<const std::uint8_t> encoded)
    {
        return decodeWithPrefix<kEd25519PublicKeySize>(KeyType::Ed25519, encoded, "ED25519");
    }

    inline EVP_PKEY* decodeKEM(std::span<const std::uint8_t> encoded)
    {
        return decodeWithPrefix<kMLKEM1024PublicKeySize>(KeyType::MLKEM1024, encoded, "ML-KEM-1024");
    }

}

#endif // ENCODEANDDECODE_H
