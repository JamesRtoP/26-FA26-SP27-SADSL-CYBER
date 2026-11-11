#include "cryptography_primatives.h"

#include <stdexcept>
#include <openssl/evp.h>

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

    EVP_PKEY* generateX25519Key()
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
        return key;
    }


    std::vector<curvePrekeyWithIdentifiers> generatePreKeysX25519(int count, int identiferStart)
    {
        std::vector<curvePrekeyWithIdentifiers> list;
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
            list.push_back({key, i + identiferStart});
        }
        return list;
    }


    EVP_PKEY* generateEd25519Key()
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
        return key;
    }

    std::array<uint8_t, 64> signWithEd25519(EVP_PKEY* key, const std::vector<uint8_t>& plaintext)
    {
        EVP_SIGNATURE* signingAlgorithm = EVP_SIGNATURE_fetch(nullptr, "ED25519", nullptr);
        if (!signingAlgorithm){throw std::runtime_error("Failed to fetch ED25519 algorithm");}

        EVP_PKEY_CTX* context = EVP_PKEY_CTX_new_from_pkey(nullptr, key, nullptr);
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

    bool checkSignWithEd25519(EVP_PKEY* key, const std::array<uint8_t,64> signature, const std::vector<uint8_t>& plaintext)
    {
        EVP_SIGNATURE* verifyingAlgorithm = EVP_SIGNATURE_fetch(nullptr, "ED25519", nullptr);
        if (!verifyingAlgorithm){throw std::runtime_error("Failed to fetch ED25519 algorithm");}

        EVP_PKEY_CTX* context = EVP_PKEY_CTX_new_from_pkey(nullptr, key, nullptr);
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


}