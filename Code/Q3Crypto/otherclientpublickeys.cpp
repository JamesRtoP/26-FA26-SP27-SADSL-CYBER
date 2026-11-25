#include "otherclientpublickeys.h"

namespace Q3Crypto
{
    otherClientPublicKeys::otherClientPublicKeys(const QJsonObject& keyBundle):IK_B(keyBundle)
    {
        verificationStep verifier(keyBundle);
        if (!verifier.checkSignatures())
            throw std::runtime_error("Key bundle signature check failed");
        this->OPK_B = {decodePublicX25519(verifier.OPK_B.key) ,verifier.OPK_B.identifier};
        this->PQPK_B = {decodePublicML_KEM_1024(verifier.PQPK_B.key), verifier.PQPK_B.identifier};
        this->SPK_B = {decodePublicX25519(verifier.SPK_B.key) ,verifier.SPK_B.identifier};

    }


    PQXDHInitialMessage otherClientPublicKeys::generateInitialMessage(IdentityKeyPair& IK_A, EVP_PKEY_X25519_ptr& EK_A, std::string plaintext, SecretData<32>& SK)
    {
        ML_KEM_1024Result CTandSS = encapsulateWithML_KEM_1024(this->PQPK_B.key);
        SecretData<32> DH_1 = Q3Crypto::generateX25519SharedSecret(IK_A.getX25519Key(), this->SPK_B.key);
        SecretData<32> DH_2 = Q3Crypto::generateX25519SharedSecret(EK_A, this->IK_B.getIK());
        SecretData<32> DH_3 = Q3Crypto::generateX25519SharedSecret(EK_A, this->SPK_B.key);
        SecretData<32> DH_4 = Q3Crypto::generateX25519SharedSecret(EK_A, this->OPK_B.key);

        SK = generateHKDFKey(concatinateKDFInputs(DH_1, DH_2, DH_3, DH_4, CTandSS.sharedSecret).span());
        std::vector<uint8_t> formattedPlaintext(plaintext.begin(), plaintext.end());
        std::vector<uint8_t> AssociatedData = generateAD(IK_A.getPublicBytes(), IK_B.getBytes(), this->PQPK_B.key);
        identitfierInfo keyIdentifiers = {this->SPK_B.identifier,this->PQPK_B.identifier, this->OPK_B.identifier};

        return {IK_A.getPublicBytes(), encodePublicX25519(EK_A), CTandSS.ciphertext, keyIdentifiers, encryptWithAES_256_GCM(SK,formattedPlaintext, AssociatedData)};

    }
}