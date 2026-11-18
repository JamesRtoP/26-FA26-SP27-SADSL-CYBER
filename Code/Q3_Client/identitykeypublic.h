#ifndef IDENTITYKEYPUBLIC_H
#define IDENTITYKEYPUBLIC_H

#include "cryptography_primatives.h"

namespace Q3Crypto
{

    class IdentityKeyPublic
    {
    private:
        EVP_PKEY_Ed25519_ptr Ed25519Key;
        EVP_PKEY_X25519_ptr X25519Key;
    public:
        IdentityKeyPublic(identityKeyPairBytes keyBytes);
        bool checkSignature(prekeyWithIdentifierSignedBytesML_KEM_1024 checkMe);
        bool checkSignature(prekeyWithIdentifierSignedBytesX25519 checkMe);
        bool checkSignature(std::vector<prekeyWithIdentifierSignedBytesML_KEM_1024>);
        bool checkSignature(std::vector<prekeyWithIdentifierSignedBytesX25519>);

    };
}
#endif // IDENTITYKEYPUBLIC_H
