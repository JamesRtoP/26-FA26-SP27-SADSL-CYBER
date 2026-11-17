#include "cryptography_primatives.h"

#include <stdexcept>
#include <openssl/evp.h>
#include <openssl/core_names.h>
#include <openssl/params.h>
#include <openssl/kdf.h>
#include <openssl/rand.h>

namespace Q3Crypto
{
    std::vector<uint8_t> hexToBytes(const std::string& hex)
    {
        std::vector<uint8_t> vector{};
        for (size_t i = 0; i < hex.size()/2; ++i) {
            // Grab 2 hex characters (e.g., "b1")
            std::string byteString = hex.substr(i * 2, 2);
            // Convert them into a real 8-bit byte (0xb1) and store it
            vector.push_back(static_cast<uint8_t>(std::stoi(byteString, nullptr, 16)));
        }
        return vector;
    }


    std::array<uint8_t, 64> sha512(const std::vector<uint8_t>& plaintext)
    {
        // fetch algorithm
        EVP_MD* messageDigest = EVP_MD_fetch(nullptr, "SHA512", nullptr);
        if (!messageDigest){throw std::runtime_error("Failed to fetch EVP_MD");}

        // create context
        EVP_MD_CTX* context = EVP_MD_CTX_new();
        if(!context)
        {
            EVP_MD_free(messageDigest);
            throw std::runtime_error("Failed to create EVP_MD_CTX");
        }

        bool success = EVP_DigestInit_ex2(context, messageDigest, nullptr) == 1;
        if(!success)
        {
            EVP_MD_CTX_free(context);
            EVP_MD_free(messageDigest);
            throw std::runtime_error("Failed to Add Algorithm to Context");
        }

        success = EVP_DigestUpdate(context, plaintext.data(), plaintext.size()) == 1;
        if(!success)
        {
            EVP_MD_CTX_free(context);
            EVP_MD_free(messageDigest);
            throw std::runtime_error("Failed to Add Message to Context");
        }

       std::array<uint8_t, 64> hash;
        unsigned int outputSize = 0;

        success = EVP_DigestFinal_ex(context, hash.data(), &outputSize) == 1;
        if(!success)
        {
            EVP_MD_CTX_free(context);
            EVP_MD_free(messageDigest);
            throw std::runtime_error("Failed to Execute Context");
        }

        EVP_MD_CTX_free(context);
        EVP_MD_free(messageDigest);

        return hash;
    }

    EVP_PKEY_X25519_ptr generateX25519Key()
    {
        EVP_PKEY_CTX* context = EVP_PKEY_CTX_new_id(EVP_PKEY_X25519, NULL);
        if(!context)
        {
            throw std::runtime_error("Failed to Generate Context");
        }
        EVP_PKEY* key = NULL;
        if(EVP_PKEY_keygen_init(context)!=1)
        {
            throw std::runtime_error("Failed to Initialize Context");
        }
        if(EVP_PKEY_keygen(context, &key)!=1)
        {
            throw std::runtime_error("Failed to Generate Key");
        }

        EVP_PKEY_X25519_ptr returnMe(key);
        return returnMe;
    }


    std::vector<prekeyWithIdentifierX25519> generatePreKeysX25519(int count, int& identiferStart)
    {
        std::vector<prekeyWithIdentifierX25519> list;
        EVP_PKEY_CTX* context = EVP_PKEY_CTX_new_id(EVP_PKEY_X25519, NULL);
        if(!context)
        {
            throw std::runtime_error("Failed to Generate Context");
        }

        for(int i = 0; i < count; i++)
        {
            EVP_PKEY* key = NULL;
            if(EVP_PKEY_keygen_init(context)!=1)
            {
                throw std::runtime_error("Failed to Initialize Context");
            }
            if(EVP_PKEY_keygen(context, &key)!=1)
            {
                throw std::runtime_error("Failed to Generate Key");
            }
            EVP_PKEY_X25519_ptr pushMe(key);
            list.push_back({std::move(pushMe), identiferStart++});
        }
        return list;
    }

    SecretData<32> generateX25519SharedSecret(EVP_PKEY_X25519_ptr peerPublicKey, EVP_PKEY_X25519_ptr privateKey)
    {
        EVP_PKEY_CTX* context = EVP_PKEY_CTX_new_from_pkey(nullptr, privateKey.get(), nullptr);
        if(!context){throw std::runtime_error("Failed to create EVP_PKEY_CTX");}

        bool success = EVP_PKEY_derive_init(context) == 1;
        if(!success)
        {
            EVP_PKEY_CTX_free(context);
            throw std::runtime_error("Failed to Initialize Context for Derivation");
        }

        success = EVP_PKEY_derive_set_peer_ex(context, peerPublicKey.get(), 1) == 1;
        if(!success)
        {
            EVP_PKEY_CTX_free(context);
            throw std::runtime_error("Failed to Add Peer Public Key to Context");
        }

        SecretData<32> sharedSecret;
        size_t secretLength = sharedSecret.size();

        success = EVP_PKEY_derive(context, sharedSecret.data(), &secretLength) == 1;
        EVP_PKEY_CTX_free(context);
        if(!success)
        {
            OPENSSL_cleanse(sharedSecret.data(), sharedSecret.size());
            throw std::runtime_error("Failed to Derive Shared Secret");
        }

        return sharedSecret;
    }


    EVP_PKEY_Ed25519_ptr generateEd25519Key()
    {
        EVP_PKEY_CTX* context = EVP_PKEY_CTX_new_id(EVP_PKEY_ED25519, NULL);
        if(!context)
        {
            throw std::runtime_error("Failed to Generate Context");
        }
        EVP_PKEY* key = NULL;
        if(EVP_PKEY_keygen_init(context)!=1)
        {
            throw std::runtime_error("Failed to Initialize Context");
        }
        if(EVP_PKEY_keygen(context, &key)!=1)
        {
            throw std::runtime_error("Failed to Generate Key");
        }
        EVP_PKEY_Ed25519_ptr returnMe(key);
        return returnMe;
    }

    std::array<uint8_t, 64> signWithEd25519(EVP_PKEY_Ed25519_ptr& key, const std::span<const std::uint8_t>& plaintext)
    {
        EVP_SIGNATURE* signingAlgorithm = EVP_SIGNATURE_fetch(nullptr, "ED25519", nullptr);
        if (!signingAlgorithm){throw std::runtime_error("Failed to fetch ED25519 algorithm");}

        EVP_PKEY_CTX* context = EVP_PKEY_CTX_new_from_pkey(nullptr, key.get(), nullptr);
        if(!context)
        {
            EVP_SIGNATURE_free(signingAlgorithm);
            throw std::runtime_error("Failed to create EVP_MD_CTX");
        }

        bool success = EVP_PKEY_sign_message_init(context, signingAlgorithm, nullptr) == 1;
        if(!success)
        {
            EVP_PKEY_CTX_free(context);
            EVP_SIGNATURE_free(signingAlgorithm);
            throw std::runtime_error("Failed to Add Algorithm to Context");
        }

        std::array<uint8_t, 64> signedStorage{};
        size_t SignatureLength;//will end us as 64

        success = EVP_PKEY_sign(context, signedStorage.data(), &SignatureLength, plaintext.data(), plaintext.size()) == 1;
        if(!success)
        {
            EVP_PKEY_CTX_free(context);
            EVP_SIGNATURE_free(signingAlgorithm);
            throw std::runtime_error("Failed to Sign to Plaintext");
        }


        EVP_PKEY_CTX_free(context);
        EVP_SIGNATURE_free(signingAlgorithm);


        return signedStorage;
    }

    bool checkSignWithEd25519(const EVP_PKEY_Ed25519_ptr& key, const std::array<uint8_t,64> signature, const std::span<const std::uint8_t>& plaintext)
    {
        EVP_SIGNATURE* verifyingAlgorithm = EVP_SIGNATURE_fetch(nullptr, "ED25519", nullptr);
        if (!verifyingAlgorithm){throw std::runtime_error("Failed to fetch ED25519 algorithm");}

        EVP_PKEY_CTX* context = EVP_PKEY_CTX_new_from_pkey(nullptr, key.get(), nullptr);
        if(!context)
        {
            EVP_SIGNATURE_free(verifyingAlgorithm);
            throw std::runtime_error("Failed to create EVP_PKEY_CTX");
        }

        bool success = EVP_PKEY_verify_message_init(context, verifyingAlgorithm, nullptr) == 1;
        if(!success)
        {
            EVP_PKEY_CTX_free(context);
            EVP_SIGNATURE_free(verifyingAlgorithm);
            throw std::runtime_error("Failed to Add Algorithm to Context");
        }

        int signatureVerificationResults = EVP_PKEY_verify(context, signature.data(), signature.size(), plaintext.data(), plaintext.size());

        EVP_PKEY_CTX_free(context);
        EVP_SIGNATURE_free(verifyingAlgorithm);

        if(signatureVerificationResults < 0)//error
        {
            throw std::runtime_error("Error during Signature Verification");
        }


        if(signatureVerificationResults != 1)// verification failed it'll return 0
        {
            return false;
        }

        //if signatureVerificationResults == 1
        return true;
    }

    SecretData<32> generateHKDFKey(const std::span<const std::uint8_t>& sharedSecret, const std::vector<uint8_t>& salt, const std::vector<uint8_t>& info)
    {
        EVP_KDF* kdfAlgorithm = EVP_KDF_fetch(nullptr, "HKDF", nullptr);
        if(!kdfAlgorithm){throw std::runtime_error("Failed to fetch HKDF algorithm");}

        EVP_KDF_CTX* context = EVP_KDF_CTX_new(kdfAlgorithm);
        if(!context)
        {
            EVP_KDF_free(kdfAlgorithm);
            throw std::runtime_error("Failed to create EVP_KDF_CTX");
        }

        char digestName[] = "SHA512";

        OSSL_PARAM parameters[5];
        size_t parameterCount = 0;

        parameters[parameterCount++] = OSSL_PARAM_construct_utf8_string(OSSL_KDF_PARAM_DIGEST, digestName, 0);
        parameters[parameterCount++] = OSSL_PARAM_construct_octet_string(OSSL_KDF_PARAM_KEY, const_cast<uint8_t*>(sharedSecret.data()), sharedSecret.size());
        if(!salt.empty())
        {
            parameters[parameterCount++] = OSSL_PARAM_construct_octet_string(OSSL_KDF_PARAM_SALT, const_cast<uint8_t*>(salt.data()), salt.size());
        }
        if(!info.empty())
        {
            parameters[parameterCount++] = OSSL_PARAM_construct_octet_string(OSSL_KDF_PARAM_INFO, const_cast<uint8_t*>(info.data()), info.size());
        }
        parameters[parameterCount] = OSSL_PARAM_construct_end();

        SecretData<32> derivedKey{};

        bool success = EVP_KDF_derive(context, derivedKey.data(), derivedKey.size(), parameters) == 1;

        EVP_KDF_CTX_free(context);
        EVP_KDF_free(kdfAlgorithm);

        if(!success)
        {
            OPENSSL_cleanse(derivedKey.data(), derivedKey.size());
            throw std::runtime_error("Failed to Derive Key with HKDF");
        }

        return derivedKey;
    }

    SecretData<32> generateHKDFKey(const std::span<const std::uint8_t>& sharedSecret)
    {
        std::vector<uint8_t> salt(64,0);
        std::string infoString = "Q3Protocol_CURVE25519_SHA-512_ML-KEM-1024";
        std::vector<uint8_t> info(infoString.begin(), infoString.end());
        return generateHKDFKey(sharedSecret, salt, info);
    }

    EVP_PKEY_ML_KEM_1024_ptr generateML_KEM_1024Key()
    {
        EVP_PKEY_CTX* context = EVP_PKEY_CTX_new_from_name(nullptr, "ML-KEM-1024", nullptr);
        if(!context){throw std::runtime_error("Failed to create EVP_PKEY_CTX");}

        bool success = EVP_PKEY_keygen_init(context) == 1;
        if(!success)
        {
            EVP_PKEY_CTX_free(context);
            throw std::runtime_error("Failed to Initialize Context for Key Generation");
        }

        EVP_PKEY* key = nullptr;
        success = EVP_PKEY_generate(context, &key) == 1;
        EVP_PKEY_CTX_free(context);

        if(!success)
        {
            EVP_PKEY_free(key);
            throw std::runtime_error("Failed to Generate ML-KEM-1024 Key");
        }
        EVP_PKEY_ML_KEM_1024_ptr returnMe(key);
        return returnMe;
    }

    std::vector<prekeyWithIdentifierML_KEM_1024> generatePreKeysML_KEM_1024(int count, int& identiferStart)
    {
        EVP_PKEY_CTX* context = EVP_PKEY_CTX_new_from_name(nullptr, "ML-KEM-1024", nullptr);
        if(!context){throw std::runtime_error("Failed to create EVP_PKEY_CTX");}

        std::vector<prekeyWithIdentifierML_KEM_1024> list;
        for(int i = 0; i <count; i++)
        {
            bool success = EVP_PKEY_keygen_init(context) == 1;
            if(!success)
            {
                EVP_PKEY_CTX_free(context);
                for(int j = 0; j < list.size(); j++)
                {
                    EVP_PKEY_free(list[j].key.release());
                }
                throw std::runtime_error("Failed to Initialize Context for Key Generation");
            }
            EVP_PKEY* key = nullptr;
            success = EVP_PKEY_generate(context, &key) == 1;
            if(!success)
            {
                EVP_PKEY_free(key);
                EVP_PKEY_CTX_free(context);
                for(int j = 0; j < list.size(); j++)
                {
                    EVP_PKEY_free(list[j].key.release());
                }
                throw std::runtime_error("Failed to Generate ML-KEM-1024 Key");
            }
            EVP_PKEY_ML_KEM_1024_ptr pushMe(key);
            list.push_back({std::move(pushMe), identiferStart++});
        }

        EVP_PKEY_CTX_free(context);

        return list;
    }

    ML_KEM_1024Result encapsulateWithML_KEM_1024(EVP_PKEY_ML_KEM_1024_ptr peerPublicKey)
    {
        EVP_PKEY_CTX* context = EVP_PKEY_CTX_new_from_pkey(nullptr, peerPublicKey.get(), nullptr);
        if(!context){throw std::runtime_error("Failed to create EVP_PKEY_CTX");}

        bool success = EVP_PKEY_encapsulate_init(context, nullptr) == 1;
        if(!success)
        {
            EVP_PKEY_CTX_free(context);
            throw std::runtime_error("Failed to Initialize Context for Encapsulation");
        }

        ML_KEM_1024Result result;

        size_t ciphertextLength = result.ciphertext.size();
        size_t secretLength = result.sharedSecret.size();

        success = EVP_PKEY_encapsulate(context, result.ciphertext.data(), &ciphertextLength, result.sharedSecret.data(), &secretLength) == 1;
        EVP_PKEY_CTX_free(context);

        if(!success)
        {
            OPENSSL_cleanse(result.sharedSecret.data(), result.sharedSecret.size());
            throw std::runtime_error("Failed to Encapsulate Shared Secret");
        }

        return result;
    }

    SecretData <32> decapsulateWithML_KEM_1024(EVP_PKEY_ML_KEM_1024_ptr privateKey, const std::array<uint8_t, 1568>& ciphertext)
    {
        EVP_PKEY_CTX* context = EVP_PKEY_CTX_new_from_pkey(nullptr, privateKey.get(), nullptr);
        if(!context){throw std::runtime_error("Failed to create EVP_PKEY_CTX");}

        bool success = EVP_PKEY_decapsulate_init(context, nullptr) == 1;
        if(!success)
        {
            EVP_PKEY_CTX_free(context);
            throw std::runtime_error("Failed to Initialize Context for Decapsulation");
        }
        SecretData<32> sharedSecret;
        size_t secretLength = sharedSecret.size();
        success = EVP_PKEY_decapsulate(context, sharedSecret.data(), &secretLength, ciphertext.data(), ciphertext.size()) == 1;
        EVP_PKEY_CTX_free(context);

        if(!success)
        {
            OPENSSL_cleanse(sharedSecret.data(), sharedSecret.size());
            throw std::runtime_error("Failed to Decapsulate Shared Secret");
        }
        return sharedSecret;

    }

    AES_256Result encryptWithAES_256_GCM(const SecretData <32>& key, const std::vector<uint8_t>& plaintext, const std::vector<uint8_t>& associatedData)
    {
        EVP_CIPHER* cipherAlgorithm = EVP_CIPHER_fetch(nullptr, "AES-256-GCM", nullptr);
        if(!cipherAlgorithm){throw std::runtime_error("Failed to fetch AES-256-GCM algorithm");}

        EVP_CIPHER_CTX* context = EVP_CIPHER_CTX_new();
        if(!context)
        {
            EVP_CIPHER_free(cipherAlgorithm);
            throw std::runtime_error("Failed to create EVP_CIPHER_CTX");
        }

        AES_256Result result;
        result.ciphertext.resize(plaintext.size() + EVP_MAX_BLOCK_LENGTH);
        int writtenLength = 0;
        int finalLength = 0;

        // fill nonce with rand bytes
        bool success = RAND_bytes(result.nonce.data(), static_cast<int>(result.nonce.size())) == 1;


        if(success)
        {
            success = EVP_EncryptInit_ex2(context, cipherAlgorithm, key.data(), result.nonce.data(), nullptr) == 1;
        }
        if(success && !associatedData.empty()) // if need to add associated Data
        {
            int associatedLength = 0;
            success = EVP_EncryptUpdate(context, nullptr, &associatedLength, associatedData.data(), static_cast<int>(associatedData.size())) == 1;
        }
        if(success && !plaintext.empty()) // if need to add plaintext
        {
            success = EVP_EncryptUpdate(context, result.ciphertext.data(), &writtenLength, plaintext.data(), static_cast<int>(plaintext.size())) == 1;
        }
        if(success)
        {
            success = EVP_EncryptFinal_ex(context, result.ciphertext.data() + writtenLength, &finalLength) == 1;
        }
        if(success)
        {
            OSSL_PARAM parameters[2];
            parameters[0] = OSSL_PARAM_construct_octet_string(OSSL_CIPHER_PARAM_AEAD_TAG, result.tag.data(), result.tag.size());
            parameters[1] = OSSL_PARAM_construct_end();
            success = EVP_CIPHER_CTX_get_params(context, parameters) == 1;
        }

        EVP_CIPHER_CTX_free(context);
        EVP_CIPHER_free(cipherAlgorithm);

        if(!success){throw std::runtime_error("Failed to Encrypt with AES-256-GCM");}
        result.ciphertext.resize(writtenLength + finalLength);

        return result;
    }

    std::vector<uint8_t> decryptWithAES_256_GCM(const SecretData <32>& key, const AES_256Result& encrypted, const std::vector<uint8_t>& associatedData)
    {
        EVP_CIPHER* cipherAlgorithm = EVP_CIPHER_fetch(nullptr, "AES-256-GCM", nullptr);
        if(!cipherAlgorithm){throw std::runtime_error("Failed to fetch AES-256-GCM algorithm");}

        EVP_CIPHER_CTX* context = EVP_CIPHER_CTX_new();
        if(!context)
        {
            EVP_CIPHER_free(cipherAlgorithm);
            throw std::runtime_error("Failed to create EVP_CIPHER_CTX");
        }
        std::vector<uint8_t> plaintext(encrypted.ciphertext.size() + EVP_MAX_BLOCK_LENGTH);
        int writtenLength = 0;
        int finalLength = 0;
        bool success = EVP_DecryptInit_ex2(context, cipherAlgorithm, key.data(), encrypted.nonce.data(), nullptr) == 1;
        if(success && !associatedData.empty())
        {
            int associatedLength = 0;
            success = EVP_DecryptUpdate(context, nullptr, &associatedLength, associatedData.data(), static_cast<int>(associatedData.size())) == 1;
        }
        if(success && !encrypted.ciphertext.empty())
        {
            success = EVP_DecryptUpdate(context, plaintext.data(), &writtenLength, encrypted.ciphertext.data(), static_cast<int>(encrypted.ciphertext.size())) == 1;
        }
        if(success)
        {
            OSSL_PARAM parameters[2];
            parameters[0] = OSSL_PARAM_construct_octet_string(OSSL_CIPHER_PARAM_AEAD_TAG, const_cast<uint8_t*>(encrypted.tag.data()), encrypted.tag.size());
            parameters[1] = OSSL_PARAM_construct_end();
            success = EVP_CIPHER_CTX_set_params(context, parameters) == 1;
        }

        bool authenticated = false;
        if(success)
        {
            authenticated = EVP_DecryptFinal_ex(context, plaintext.data() + writtenLength, &finalLength) == 1;
        }

        EVP_CIPHER_CTX_free(context);
        EVP_CIPHER_free(cipherAlgorithm);

        if(!success)
        {
            OPENSSL_cleanse(plaintext.data(), plaintext.size());
            throw std::runtime_error("Failed to Decrypt with AES-256-GCM");
        }

        if(!authenticated)
        {
            OPENSSL_cleanse(plaintext.data(), plaintext.size());
            throw std::runtime_error("Authentication Failed: key, nonce, tag, ciphertext or associated data does not match");
        }

        plaintext.resize(writtenLength + finalLength);
        return plaintext;
    }

    std::vector<uint8_t> getPublicKeyBytes(EVP_PKEY* key)
    {
        if(!key){throw std::invalid_argument("Key is null");}

        size_t publicKeyLength = 0;
        bool success = EVP_PKEY_get_octet_string_param(key, OSSL_PKEY_PARAM_PUB_KEY, nullptr, 0, &publicKeyLength) == 1;
        if(!success){throw std::runtime_error("Failed to get Public Key Length");}

        if(publicKeyLength == 0){throw std::runtime_error("Public Key is Empty");}

        std::vector<uint8_t> publicKey(publicKeyLength);

        success = EVP_PKEY_get_octet_string_param(key, OSSL_PKEY_PARAM_PUB_KEY, publicKey.data(), publicKey.size(), &publicKeyLength) == 1;
        if(!success){throw std::runtime_error("Failed to get Public Key");}

        if(publicKeyLength != publicKey.size())
        {
            throw std::runtime_error("Unexpected Public Key Length");
        }

        return publicKey;
    }

    std::array<std::uint8_t, PublicKeySizeX25519 + 1> encodePublicX25519(const EVP_PKEY_X25519_ptr& key)
    {
        if (key == nullptr)
            throw std::invalid_argument("encode: null X25519 key");
        if (!EVP_PKEY_is_a(key.get(), "X25519"))
            throw std::invalid_argument("encode: key is not X25519");

        std::array<std::uint8_t, PublicKeySizeX25519 + 1> encoded;
        encoded.data()[0] = 0;

        std::size_t length = PublicKeySizeX25519;
        if (EVP_PKEY_get_raw_public_key(key.get(), encoded.data() + 1, &length) != 1 || length != PublicKeySizeX25519)
            throw std::runtime_error("encode: failed to export X25519 public key");
        return encoded;
    }

    std::array<std::uint8_t, PublicKeySizeEd25519 + 1> encodePublicEd25519(const EVP_PKEY_Ed25519_ptr& key)
    {
        if (key == nullptr)
            throw std::invalid_argument("encode: null ED25519 key");
        if (!EVP_PKEY_is_a(key.get(), "ED25519"))
            throw std::invalid_argument("encode: key is not ED25519");

        std::array<std::uint8_t, PublicKeySizeEd25519 + 1> encoded;
        encoded.data()[0] = 10;

        std::size_t length = PublicKeySizeEd25519;
        if (EVP_PKEY_get_raw_public_key(key.get(), encoded.data() + 1, &length) != 1 || length != PublicKeySizeEd25519)
            throw std::runtime_error("encode: failed to export ED25519 public key");
        return encoded;
    }

    std::array<std::uint8_t, PublicKeySizeML_KEM_1024 + 1> encodePublicML_KEM_1024(const EVP_PKEY_ML_KEM_1024_ptr& key)
    {
        if (key == nullptr)
            throw std::invalid_argument("encode: null ML-KEM-1024 key");
        if (!EVP_PKEY_is_a(key.get(), "ML-KEM-1024"))
            throw std::invalid_argument("encode: key is not ML-KEM-1024");

        std::array<std::uint8_t, PublicKeySizeML_KEM_1024 + 1> encoded;
        encoded.data()[0] = 20;

        std::size_t length = PublicKeySizeML_KEM_1024;
        if (EVP_PKEY_get_raw_public_key(key.get(), encoded.data() + 1, &length) != 1 || length != PublicKeySizeML_KEM_1024)
            throw std::runtime_error("encode: failed to export ML-KEM-1024 public key");
        return encoded;
    }

    EVP_PKEY_X25519_ptr decodePublicX25519(std::array<std::uint8_t, PublicKeySizeX25519 + 1> encoded)
    {
        if (encoded.empty())
            throw std::runtime_error("decode X25519: encoded key is empty");
        if (encoded[0] != 0)
            throw std::runtime_error("decode X25519: unrecognised key type byte "
                                     + std::to_string(encoded[0]));
        if (encoded.size() != PublicKeySizeX25519 + 1)
            throw std::runtime_error("decode X25519: expected " + std::to_string(PublicKeySizeX25519 + 1)
                                     + " bytes, got " + std::to_string(encoded.size()));

        EVP_PKEY* key(
            EVP_PKEY_new_raw_public_key_ex(nullptr, "X25519", nullptr, encoded.data() + 1, PublicKeySizeX25519));
        if (!key)
            throw std::runtime_error("decode X25519: OpenSSL rejected the key");
        EVP_PKEY_X25519_ptr returnMe(key);
        return returnMe;
    }

    EVP_PKEY_Ed25519_ptr decodePublicEd25519(std::array<std::uint8_t, PublicKeySizeEd25519 + 1> encoded)
    {

        if (encoded.empty())
            throw std::runtime_error("decode ED25519: encoded key is empty");
        if (encoded[0] != 10)
            throw std::runtime_error("decode ED25519: unrecognised key type byte "
                                     + std::to_string(encoded[0]));
        if (encoded.size() != PublicKeySizeEd25519 + 1)
            throw std::runtime_error("decode ED25519: expected " + std::to_string(PublicKeySizeEd25519 + 1)
                                     + " bytes, got " + std::to_string(encoded.size()));

        EVP_PKEY* key(
            EVP_PKEY_new_raw_public_key_ex(nullptr, "ED25519", nullptr, encoded.data() + 1, PublicKeySizeEd25519));
        if (!key)
            throw std::runtime_error("decode ED25519: OpenSSL rejected the key");
        EVP_PKEY_Ed25519_ptr returnMe(key);
        return returnMe;
    }

    EVP_PKEY_ML_KEM_1024_ptr decodePublicML_KEM_1024(std::array<std::uint8_t, PublicKeySizeML_KEM_1024 + 1> encoded)
    {

        if (encoded.empty())
            throw std::runtime_error("decode ML-KEM-1024: encoded key is empty");
        if (encoded[0] != 20)
            throw std::runtime_error("decode ML-KEM-1024: unrecognised key type byte "
                                     + std::to_string(encoded[0]));
        if (encoded.size() != PublicKeySizeML_KEM_1024 + 1)
            throw std::runtime_error("decode ML-KEM-1024: expected " + std::to_string(PublicKeySizeML_KEM_1024 + 1)
                                     + " bytes, got " + std::to_string(encoded.size()));

        EVP_PKEY* key(
            EVP_PKEY_new_raw_public_key_ex(nullptr, "ML-KEM-1024", nullptr, encoded.data() + 1, PublicKeySizeML_KEM_1024));
        if (!key)
            throw std::runtime_error("decode ML-KEM-1024: OpenSSL rejected the key");
        EVP_PKEY_ML_KEM_1024_ptr returnMe(key);
        return returnMe;
    }

    SecretData<PrivateKeySizeX25519 + 1> encodePrivateX25519(const EVP_PKEY_X25519_ptr& key)
    {
        if (key == nullptr)
            throw std::invalid_argument("encode: null X25519 key");
        if (!EVP_PKEY_is_a(key.get(), "X25519"))
            throw std::invalid_argument("encode: key is not X25519");

        SecretData<PrivateKeySizeX25519 + 1> encoded;
        encoded.data()[0] = 1;

        std::size_t length = PrivateKeySizeX25519;
        if (EVP_PKEY_get_raw_private_key(key.get(), encoded.data() + 1, &length) != 1 || length != PrivateKeySizeX25519)
            throw std::runtime_error("encode: failed to export X25519 public key");
        return encoded;
    }

    SecretData<PrivateKeySizeEd25519 + 1> encodePrivateEd25519(const EVP_PKEY_Ed25519_ptr& key)
    {
        if (key == nullptr)
            throw std::invalid_argument("encode: null ED25519 key");
        if (!EVP_PKEY_is_a(key.get(), "ED25519"))
            throw std::invalid_argument("encode: key is not ED25519");

        SecretData<PrivateKeySizeEd25519 + 1> encoded;
        encoded.data()[0] = 11;

        std::size_t length = PrivateKeySizeEd25519;
        if (EVP_PKEY_get_raw_public_key(key.get(), encoded.data() + 1, &length) != 1 || length != PrivateKeySizeEd25519)
            throw std::runtime_error("encode: failed to export ED25519 public key");
        return encoded;
    }

    SecretData<PrivateKeySizeML_KEM_1024 + 1> encodePrivateML_KEM_1024(const EVP_PKEY_ML_KEM_1024_ptr& key)
    {
        if (key == nullptr)
            throw std::invalid_argument("encode: null ML-KEM-1024 key");
        if (!EVP_PKEY_is_a(key.get(), "ML-KEM-1024"))
            throw std::invalid_argument("encode: key is not ML-KEM-1024");

        SecretData<PrivateKeySizeML_KEM_1024 + 1> encoded;
        encoded.data()[0] = 21;

        std::size_t length = PrivateKeySizeML_KEM_1024;
        if (EVP_PKEY_get_octet_string_param(key.get(), OSSL_PKEY_PARAM_ML_KEM_SEED,  encoded.data() + 1, PrivateKeySizeML_KEM_1024, &length) != 1 || length != PrivateKeySizeML_KEM_1024)
            throw std::runtime_error("encode: failed to export ML-KEM-1024 public key");
        return encoded;
    }

    EVP_PKEY_X25519_ptr decodePrivateX25519(SecretData<PrivateKeySizeX25519 + 1> encoded)
    {
        if (encoded.data()[0] != 1)
            throw std::runtime_error("decode X25519: unrecognised key type byte "
                                     + std::to_string(encoded.data()[0]));

        EVP_PKEY* key(
            EVP_PKEY_new_raw_private_key_ex(nullptr, "X25519", nullptr, encoded.data() + 1, PrivateKeySizeX25519));
        if (!key)
            throw std::runtime_error("decode X25519: OpenSSL rejected the key");
        EVP_PKEY_X25519_ptr returnMe(key);
        return returnMe;
    }

    EVP_PKEY_Ed25519_ptr decodePrivateEd25519(SecretData<PrivateKeySizeEd25519 + 1> encoded)
    {
        if (encoded.data()[0] != 11)
            throw std::runtime_error("decode ED25519: unrecognised key type byte "
                                     + std::to_string(encoded.data()[0]));

        EVP_PKEY* key(
            EVP_PKEY_new_raw_private_key_ex(nullptr, "ED25519", nullptr, encoded.data() + 1, PrivateKeySizeEd25519));
        if (!key)
            throw std::runtime_error("decode ED25519: OpenSSL rejected the key");
        EVP_PKEY_Ed25519_ptr returnMe(key);
        return returnMe;
    }

    EVP_PKEY_ML_KEM_1024_ptr decodePrivateML_KEM_1024(const SecretData<PrivateKeySizeML_KEM_1024 + 1>& encoded)
    {
        if (encoded.data()[0] != 21)
            throw std::runtime_error("decode ML-KEM-1024 private: unrecognised key type byte "
                                     + std::to_string(encoded.data()[0]));

        EVP_PKEY_CTX* context = EVP_PKEY_CTX_new_from_name(nullptr, "ML-KEM-1024", nullptr);
        if (!context)
            throw std::runtime_error("decode ML-KEM-1024 private: failed to create context");

        if (EVP_PKEY_keygen_init(context) != 1)
            EVP_PKEY_CTX_free(context);
            throw std::runtime_error("decode ML-KEM-1024 private: failed to initialise keygen");

        OSSL_PARAM parameters[2];
        parameters[0] = OSSL_PARAM_construct_octet_string(OSSL_PKEY_PARAM_ML_KEM_SEED,
                                                          const_cast<std::uint8_t*>(encoded.data() + 1),
                                                          PrivateKeySizeML_KEM_1024);
        parameters[1] = OSSL_PARAM_construct_end();

        if (EVP_PKEY_CTX_set_params(context, parameters) != 1)
            EVP_PKEY_CTX_free(context);
            throw std::runtime_error("decode ML-KEM-1024 private: OpenSSL rejected the seed");

        EVP_PKEY* key = nullptr;
        if (EVP_PKEY_generate(context, &key) != 1)
            EVP_PKEY_CTX_free(context);
            throw std::runtime_error("decode ML-KEM-1024 private: failed to regenerate key from seed");

        EVP_PKEY_CTX_free(context);
        EVP_PKEY_ML_KEM_1024_ptr returnMe(key);
        return returnMe;
    }
}