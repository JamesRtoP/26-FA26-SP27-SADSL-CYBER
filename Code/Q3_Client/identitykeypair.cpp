#include "identitykeypair.h"
#include <stdexcept>

namespace Q3Crypto
{
    IdentityKeyPair::IdentityKeyPair()
    {
        this->Ed25519Key = generateEd25519Key();
        this->X25519Key = generateX25519Key();
    }

    IdentityKeyPair::IdentityKeyPair(identityKeyPairBytes IDBytes)
    {
        this->Ed25519Key = decodeEd25519(IDBytes.X25519_key);
        this->X25519Key = decodeX25519(IDBytes.X25519_key);
    }

    identityKeyPairBytes IdentityKeyPair::getBytes()
    {
        identityKeyPairBytes returnMe;
        returnMe.Ed25519_key = encodeEd25519(this->Ed25519Key);
        returnMe.X25519_key = encodeX25519(this->X25519Key);
        return returnMe;
    }

    prekeyBytesWithIdentifierSignedML_KEM_1024 IdentityKeyPair::signPublicPrekeyML_KEM_1024(prekeyWithIdentifierML_KEM_1024 keyWithID)
    {
        prekeyBytesWithIdentifierSignedML_KEM_1024 returnMe;
        returnMe.identifier = keyWithID.identifier;
        returnMe.key = encodeML_KEM_1024(keyWithID.key);
        returnMe.signature = signWithEd25519(this->Ed25519Key, encodeML_KEM_1024(keyWithID.key));
        return returnMe;
    }

    prekeyBytesWithIdentifierSignedX25519 IdentityKeyPair::signPublicPrekeyX25519(prekeyWithIdentifierX25519 keyWithID)
    {
        prekeyBytesWithIdentifierSignedX25519 returnMe;
        returnMe.identifier = keyWithID.identifier;
        returnMe.key = encodeX25519(keyWithID.key);
        returnMe.signature = signWithEd25519(this->Ed25519Key, encodeX25519(keyWithID.key));
        return returnMe;
    }

    // Redo signing algorithm for speed of signing
    std::vector<prekeyBytesWithIdentifierSignedML_KEM_1024> IdentityKeyPair::signBulkPublicPrekeysML_KEM_1024(std::vector<prekeyWithIdentifierML_KEM_1024> inList)
    {
        std::vector<prekeyBytesWithIdentifierSignedML_KEM_1024> outList;

        EVP_SIGNATURE* signingAlgorithm = EVP_SIGNATURE_fetch(nullptr, "ED25519", nullptr);
        if (!signingAlgorithm){throw std::runtime_error("Failed to fetch ED25519 algorithm");}

        EVP_PKEY_CTX* context = EVP_PKEY_CTX_new_from_pkey(nullptr, this->Ed25519Key.get(), nullptr);
        if(!context)
        {
            EVP_SIGNATURE_free(signingAlgorithm);
            throw std::runtime_error("Failed to create EVP_MD_CTX");
        }

        for(int i = 0; i < inList.size(); i++)
        {
            bool success = EVP_PKEY_sign_message_init(context, signingAlgorithm, nullptr) == 1;
            if(!success)
            {
                EVP_PKEY_CTX_free(context);
                EVP_SIGNATURE_free(signingAlgorithm);
                throw std::runtime_error("Failed to Add Algorithm to Context");
            }

            std::array<uint8_t, 64> signedStorage{};
            size_t SignatureLength;//will end us as 64

            std::array<std::uint8_t, PublicKeySizeML_KEM_1024 + 1> plaintext = encodeML_KEM_1024(inList[i].key);

            success = EVP_PKEY_sign(context, signedStorage.data(), &SignatureLength, plaintext.data(), plaintext.size()) == 1;
            if(!success)
            {
                EVP_PKEY_CTX_free(context);
                EVP_SIGNATURE_free(signingAlgorithm);
                throw std::runtime_error("Failed to Sign to Plaintext");
            }

            outList.push_back({encodeML_KEM_1024(inList[i].key), inList[i].identifier, signedStorage});
        }


        EVP_PKEY_CTX_free(context);
        EVP_SIGNATURE_free(signingAlgorithm);


        return outList;
    }

    std::vector<prekeyBytesWithIdentifierSignedX25519> IdentityKeyPair::signBulkPublicPrekeysX25519(std::vector<prekeyWithIdentifierX25519> inList)
    {
        std::vector<prekeyBytesWithIdentifierSignedX25519> outList;

        EVP_SIGNATURE* signingAlgorithm = EVP_SIGNATURE_fetch(nullptr, "ED25519", nullptr);
        if (!signingAlgorithm){throw std::runtime_error("Failed to fetch ED25519 algorithm");}

        EVP_PKEY_CTX* context = EVP_PKEY_CTX_new_from_pkey(nullptr, this->Ed25519Key.get(), nullptr);
        if(!context)
        {
            EVP_SIGNATURE_free(signingAlgorithm);
            throw std::runtime_error("Failed to create EVP_MD_CTX");
        }

        for(int i = 0; i < inList.size(); i++)
        {
            bool success = EVP_PKEY_sign_message_init(context, signingAlgorithm, nullptr) == 1;
            if(!success)
            {
                EVP_PKEY_CTX_free(context);
                EVP_SIGNATURE_free(signingAlgorithm);
                throw std::runtime_error("Failed to Add Algorithm to Context");
            }

            std::array<uint8_t, 64> signedStorage{};
            size_t SignatureLength;//will end us as 64

            std::array<std::uint8_t, PublicKeySizeX25519 + 1> plaintext = encodeX25519(inList[i].key);

            success = EVP_PKEY_sign(context, signedStorage.data(), &SignatureLength, plaintext.data(), plaintext.size()) == 1;
            if(!success)
            {
                EVP_PKEY_CTX_free(context);
                EVP_SIGNATURE_free(signingAlgorithm);
                throw std::runtime_error("Failed to Sign to Plaintext");
            }

            outList.push_back({encodeX25519(inList[i].key), inList[i].identifier, signedStorage});
        }


        EVP_PKEY_CTX_free(context);
        EVP_SIGNATURE_free(signingAlgorithm);


        return outList;
    }
}
