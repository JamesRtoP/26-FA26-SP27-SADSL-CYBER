#ifndef OTHERCLIENTPUBLICKEYS_H
#define OTHERCLIENTPUBLICKEYS_H

#include "sharedKeyBytesStorage.h"
#include "identitykeypublic.h"
#include "pqxdh_functions.h"

namespace Q3Crypto
{
    struct identitfierInfo
    {
        int SPK_B_Identifier;
        int PQPK_B_Identifier;
        int OPK_B_Identifier;
    };

    struct PQXDHInitialMessage
    {
        identityKeyPublicBytes IK_B;
        std::array<uint8_t, PublicKeySizeX25519 + 1> EK_B;
        std::array<uint8_t, 1568> PQKEMCiphertext;
        identitfierInfo keyIdentifiers;
        AES_256Result initialCiphertext;
    };

    class otherClientPublicKeys
    {
    private:
        IdentityKeyPublic IK_B;
        prekeyWithIdentifierX25519 SPK_B;
        prekeyWithIdentifierML_KEM_1024 PQPK_B;
        prekeyWithIdentifierX25519 OPK_B;


    public:
        otherClientPublicKeys();
        PQXDHInitialMessage generateInitialMessage(IdentityKeyPair& IK_A, EVP_PKEY_X25519_ptr& EK_A, std::string plaintext, SecretData<32>& SK);
    };
}

#endif // OTHERCLIENTPUBLICKEYS_H
