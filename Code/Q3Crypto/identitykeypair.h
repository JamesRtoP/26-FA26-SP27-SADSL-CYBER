#ifndef IDENTITYKEYPAIR_H
#define IDENTITYKEYPAIR_H

#include "cryptography_primatives.h"

namespace Q3Crypto
{
    class IdentityKeyPair
    {
    private:
        EVP_PKEY_Ed25519_ptr Ed25519Key;
        EVP_PKEY_X25519_ptr X25519Key;

    public:
        IdentityKeyPair();
        IdentityKeyPair(identityKeyPublicBytes IDBytes);
        identityKeyPublicBytes getPublicBytes();
        prekeyWithIdentifierSignedBytesML_KEM_1024 signPublicPrekeyML_KEM_1024(const prekeyWithIdentifierML_KEM_1024& keyWithID);
        prekeyWithIdentifierSignedBytesX25519 signPublicPrekeyX25519(const prekeyWithIdentifierX25519& keyWithID);
        std::vector<prekeyWithIdentifierSignedBytesML_KEM_1024> signBulkPublicPrekeysML_KEM_1024(const std::vector<prekeyWithIdentifierML_KEM_1024>&);
        const EVP_PKEY_X25519_ptr& getX25519Key();
    };
}
#endif // IDENTITYKEYPAIR_H

