#ifndef IDENTITYKEYPUBLIC_H
#define IDENTITYKEYPUBLIC_H

#include "identitykeypair.h"
namespace Q3Crypto
{

    class IdentityKeyPublic
    {
    private:
        EVP_PKEY_Ed25519_ptr Ed25519Key;
        EVP_PKEY_X25519_ptr X25519Key;
    public:
        IdentityKeyPublic(identityKeyPublicBytes keyBytes);
        IdentityKeyPublic(const IdentityKeyPair& keyBytes);

        bool checkSignature(prekeyWithIdentifierSignedBytesML_KEM_1024 checkMe);
        bool checkSignature(prekeyWithIdentifierSignedBytesX25519 checkMe);
        bool checkSignature(std::vector<prekeyWithIdentifierSignedBytesML_KEM_1024>);
        bool checkSignature(std::vector<prekeyWithIdentifierSignedBytesX25519>);
        const EVP_PKEY_X25519_ptr& getIK();
        identityKeyPublicBytes const getBytes();
    };
}
#endif // IDENTITYKEYPUBLIC_H
