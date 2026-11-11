#include "cryptography_primatives.h"

#include <stdexcept>
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

}