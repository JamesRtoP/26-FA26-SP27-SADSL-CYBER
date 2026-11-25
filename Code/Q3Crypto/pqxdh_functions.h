#ifndef PQXDH_FUNCTIONS_H
#define PQXDH_FUNCTIONS_H

#include "cryptography_primatives.h"
#include "identitykeypublic.h"
#include "SecretData.h"

namespace Q3Crypto
{
    SecretData<160> concatinateKDFInputs(SecretData<32>& DH_1,SecretData<32>& DH_2,SecretData<32>& DH_3,SecretData<32>& DH_4,SecretData<32>& SS);
    std::vector<uint8_t> generateAD(const identityKeyPublicBytes& IK_A_bytes, const identityKeyPublicBytes& IK_B_bytes, const EVP_PKEY_ML_KEM_1024_ptr& PQPK_B);

    struct identitfierInfo
    {
        int SPK_B_Identifier;
        int PQPK_B_Identifier;
        int OPK_B_Identifier;
    };

    struct PQXDHInitialMessage
    {
        identityKeyPublicBytes IK_A;
        std::array<uint8_t, PublicKeySizeX25519 + 1> EK_A;
        std::array<uint8_t, 1568> PQKEMCiphertext;
        identitfierInfo keyIdentifiers;
        AES_256Result initialCiphertext;
    };

    QJsonObject initialMessageToJson(const PQXDHInitialMessage& message);
    PQXDHInitialMessage initialMessageFromJson(const QJsonObject& json);
}
#endif // PQXDH_FUNCTIONS_H
