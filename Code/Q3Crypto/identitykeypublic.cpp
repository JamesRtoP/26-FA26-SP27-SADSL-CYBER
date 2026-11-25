#include "identitykeypublic.h"
#include "stdexcept"

namespace Q3Crypto
{

    IdentityKeyPublic::IdentityKeyPublic(identityKeyPublicBytes keyBytes)
    {
        this->Ed25519Key = decodePublicEd25519(keyBytes.Ed25519_key);
        this->X25519Key = decodePublicX25519(keyBytes.X25519_key);
    }

    IdentityKeyPublic::IdentityKeyPublic(const IdentityKeyPair& keyBytes)
    {

    }

    IdentityKeyPublic::IdentityKeyPublic(const QJsonObject& keys)
    {
        identityKeyPublicBytes IDBytes;
        base64FieldToBytes(keys, "identityKey", IDBytes.X25519_key.data(), IDBytes.X25519_key.size());
        base64FieldToBytes(keys, "identitySigningKey", IDBytes.Ed25519_key.data(), IDBytes.Ed25519_key.size());
        this->Ed25519Key = decodePublicEd25519(IDBytes.Ed25519_key);
        this->X25519Key = decodePublicX25519(IDBytes.X25519_key);
    }

    bool IdentityKeyPublic::checkSignature(prekeyWithIdentifierSignedBytesML_KEM_1024 checkMe)
    {
        return checkSignWithEd25519(this->Ed25519Key, checkMe.signature, checkMe.key);
    }

    bool IdentityKeyPublic::checkSignature(prekeyWithIdentifierSignedBytesX25519 checkMe)
    {
        return checkSignWithEd25519(this->Ed25519Key, checkMe.signature, checkMe.key);
    }

    bool IdentityKeyPublic::checkSignature(std::vector<prekeyWithIdentifierSignedBytesML_KEM_1024> list)
    {
        EVP_SIGNATURE* verifyingAlgorithm = EVP_SIGNATURE_fetch(nullptr, "ED25519", nullptr);
        if (!verifyingAlgorithm){throw std::runtime_error("Failed to fetch ED25519 algorithm");}

        EVP_PKEY_CTX* context = EVP_PKEY_CTX_new_from_pkey(nullptr, this->Ed25519Key.get(), nullptr);
        if(!context)
        {
            EVP_SIGNATURE_free(verifyingAlgorithm);
            throw std::runtime_error("Failed to create EVP_PKEY_CTX");
        }

        for(int i = 0; i < list.size(); i++)
        {

            bool success = EVP_PKEY_verify_message_init(context, verifyingAlgorithm, nullptr) == 1;
            if(!success)
            {
                EVP_PKEY_CTX_free(context);
                EVP_SIGNATURE_free(verifyingAlgorithm);
                throw std::runtime_error("Failed to Add Algorithm to Context");
            }

            int signatureVerificationResults = EVP_PKEY_verify(context, list[i].signature.data(), list[i].signature.size(), list[i].key.data(), list[i].key.size());

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
        }

        //if signatureVerificationResults == 1
        return true;
    }

    bool IdentityKeyPublic::checkSignature(std::vector<prekeyWithIdentifierSignedBytesX25519>list)
    {
        EVP_SIGNATURE* verifyingAlgorithm = EVP_SIGNATURE_fetch(nullptr, "ED25519", nullptr);
        if (!verifyingAlgorithm){throw std::runtime_error("Failed to fetch ED25519 algorithm");}

        EVP_PKEY_CTX* context = EVP_PKEY_CTX_new_from_pkey(nullptr, this->Ed25519Key.get(), nullptr);
        if(!context)
        {
            EVP_SIGNATURE_free(verifyingAlgorithm);
            throw std::runtime_error("Failed to create EVP_PKEY_CTX");
        }

        for(int i = 0; i < list.size(); i++)
        {

            bool success = EVP_PKEY_verify_message_init(context, verifyingAlgorithm, nullptr) == 1;
            if(!success)
            {
                EVP_PKEY_CTX_free(context);
                EVP_SIGNATURE_free(verifyingAlgorithm);
                throw std::runtime_error("Failed to Add Algorithm to Context");
            }

            int signatureVerificationResults = EVP_PKEY_verify(context, list[i].signature.data(), list[i].signature.size(), list[i].key.data(), list[i].key.size());

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
        }

        //if signatureVerificationResults == 1
        return true;
    }

    const EVP_PKEY_X25519_ptr& IdentityKeyPublic::getIK()
    {
        return this->X25519Key;
    }

    identityKeyPublicBytes const IdentityKeyPublic::getBytes()
    {
        identityKeyPublicBytes returnMe;
        returnMe.Ed25519_key = encodePublicEd25519(this->Ed25519Key);
        returnMe.X25519_key = encodePublicX25519(this->X25519Key);
        return returnMe;
    }

}