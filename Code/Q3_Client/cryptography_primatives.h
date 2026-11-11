#ifndef CRYPTOGRAPHY_PRIMATIVES_H
#define CRYPTOGRAPHY_PRIMATIVES_H

#include <openssl/evp.h>

#include <vector>
#include <array>
#include <string>

namespace Q3Crypto
{
    std::vector<uint8_t> hexToBytes(const std::string& hex);
    std::array<uint8_t, 64> sha512(const std::vector<uint8_t>& plaintext);
}

#endif // CRYPTOGRAPHY_PRIMATIVES_H