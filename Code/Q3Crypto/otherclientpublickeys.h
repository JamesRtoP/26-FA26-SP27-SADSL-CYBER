#ifndef OTHERCLIENTPUBLICKEYS_H
#define OTHERCLIENTPUBLICKEYS_H

#include "sharedKeyBytesStorage.h"
#include "identitykeypublic.h"
#include "pqxdh_functions.h"
#include "pqxdh_functions.h"
#include <QJsonObject>

namespace Q3Crypto
{
    class otherClientPublicKeys
    {
    private:
        struct verificationStep
        {
            IdentityKeyPublic IK_B;
            prekeyWithIdentifierSignedBytesX25519 SPK_B;
            prekeyWithIdentifierSignedBytesML_KEM_1024 PQPK_B;
            prekeyWithIdentifierBytesX25519 OPK_B;
            verificationStep(const QJsonObject& keyBundle):IK_B(keyBundle)
            {
                this->SPK_B = curvePrekeySignedFromJson(keyBundle.value("signedPreKey").toObject());
                this->PQPK_B = pqkemPrekeySignedFromJson(keyBundle.value("pqSignedPreKey").toObject());
                this->OPK_B = curvePrekeyFromJson(keyBundle.value("oneTimePreKey").toObject());
            }
            bool checkSignatures()
            {
                return this->IK_B.checkSignature(this->SPK_B) && this->IK_B.checkSignature(this->PQPK_B);
            }
        };

        IdentityKeyPublic IK_B;
        prekeyWithIdentifierX25519 SPK_B;
        prekeyWithIdentifierML_KEM_1024 PQPK_B;
        prekeyWithIdentifierX25519 OPK_B;

    public:
        otherClientPublicKeys(const QJsonObject& keyBundle);
        PQXDHInitialMessage generateInitialMessage(IdentityKeyPair& IK_A, EVP_PKEY_X25519_ptr& EK_A, std::string plaintext, SecretData<32>& SK);
    };
}

#endif // OTHERCLIENTPUBLICKEYS_H
