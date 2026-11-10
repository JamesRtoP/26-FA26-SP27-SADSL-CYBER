#include "cryptography_primatives.h"

#include <stdexcept>

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