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
        IdentityKeyPair(identityKeyPairBytes IDBytes);
        identityKeyPairBytes getBytes();
        prekeyWithIdentifierSignedBytesML_KEM_1024 signPublicPrekeyML_KEM_1024(const prekeyWithIdentifierML_KEM_1024& keyWithID);
        prekeyWithIdentifierSignedBytesX25519 signPublicPrekeyX25519(const prekeyWithIdentifierX25519& keyWithID);
        std::vector<prekeyWithIdentifierSignedBytesML_KEM_1024> signBulkPublicPrekeysML_KEM_1024(const std::vector<prekeyWithIdentifierML_KEM_1024>&);
        std::vector<prekeyWithIdentifierSignedBytesX25519> signBulkPublicPrekeysX25519(const std::vector<prekeyWithIdentifierX25519>&);
    };
}
#endif // IDENTITYKEYPAIR_H

