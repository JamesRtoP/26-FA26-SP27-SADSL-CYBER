#include "clientkeystorage.h"
#include "sharedKeyBytesStorage.h"

namespace Q3Crypto
{

    std::vector<prekeyWithIdentifierBytesX25519> ClientKeyStorage::getCurveBulkPrekeyBytes()
    {
        std::vector<prekeyWithIdentifierBytesX25519> returnMe;
        for(int i = 0; i < this->curveBulkPrekeys.size(); i++)
        {
            returnMe.push_back({encodePublicX25519(this->curveBulkPrekeys[i].key),this->curveBulkPrekeys[i].identifier});
        }
        return returnMe;
    }

    ClientKeyStorage::ClientKeyStorage()
    {
        this->curvePrekey = {generateX25519Key(), 0};
        this->lastResortpqkemKey = {generateML_KEM_1024Key(),100};

        this->nextCurveBulkPrekeysIdentifier = 10000;
        this->curveBulkPrekeys = generatePreKeysX25519(50, this->nextCurveBulkPrekeysIdentifier);

        this->nextpqkemBulkPrekeysIdentifier = 20000;
        this->pqkemBulkPrekeys = generatePreKeysML_KEM_1024(50, this->nextpqkemBulkPrekeysIdentifier);
    }


    SharedKeyBytesStorage ClientKeyStorage::getKeyBytes()
    {
        identityKeyPublicBytes IDKeyBytes = this->identity_key.getPublicBytes();
        prekeyWithIdentifierSignedBytesX25519 new_curvePrekey = this->identity_key.signPublicPrekeyX25519(this->curvePrekey);
        prekeyWithIdentifierSignedBytesML_KEM_1024 new_lastResortpqkemKey = this->identity_key.signPublicPrekeyML_KEM_1024(this->lastResortpqkemKey);
        std::vector<prekeyWithIdentifierBytesX25519> new_curveBulkPrekeys = this->getCurveBulkPrekeyBytes();
        std::vector<prekeyWithIdentifierSignedBytesML_KEM_1024> new_pqkemBulkPrekeys = this->identity_key.signBulkPublicPrekeysML_KEM_1024(this->pqkemBulkPrekeys);
        return SharedKeyBytesStorage (IDKeyBytes, new_curvePrekey, new_lastResortpqkemKey, new_curveBulkPrekeys, new_pqkemBulkPrekeys);

    }

    IdentityKeyPair& ClientKeyStorage::getIdentityKey()
    {
        return this->identity_key;
    }

    SecretData<32> ClientKeyStorage::getSKFromInitialMessage(const PQXDHInitialMessage message)
    {
        IdentityKeyPublic IK_A(message.IK_A);
        EVP_PKEY_X25519_ptr EK_A = decodePublicX25519(message.EK_A);

        if(this->curvePrekey.identifier != message.keyIdentifiers.SPK_B_Identifier)
            throw std::runtime_error("Unknown one-time prekey ID");
        EVP_PKEY_X25519_ptr& SPK_B = this->curvePrekey.key;

        IdentityKeyPair& IK_B = this->identity_key;//.getX25519Key()
        auto OPK_B_iterator = std::find_if(this->curveBulkPrekeys.begin(), this->curveBulkPrekeys.end(),
                                                  [id = message.keyIdentifiers.OPK_B_Identifier] (prekeyWithIdentifierX25519& list){return list.identifier == id;});
        if (OPK_B_iterator == this->curveBulkPrekeys.end())
            throw std::runtime_error("Unknown one-time curve prekey ID");
        EVP_PKEY_X25519_ptr& OPK_B = OPK_B_iterator->key;

        SecretData<32> DH_1 = Q3Crypto::generateX25519SharedSecret(SPK_B, IK_A.getIK());
        SecretData<32> DH_2 = Q3Crypto::generateX25519SharedSecret(IK_B.getX25519Key(), EK_A);
        SecretData<32> DH_3 = Q3Crypto::generateX25519SharedSecret(SPK_B, EK_A);
        SecretData<32> DH_4 = Q3Crypto::generateX25519SharedSecret(OPK_B, EK_A);


        auto PQPK_B_iterator = std::find_if(this->pqkemBulkPrekeys.begin(), this->pqkemBulkPrekeys.end(),
                                  [id = message.keyIdentifiers.PQPK_B_Identifier] (prekeyWithIdentifierML_KEM_1024& list){return list.identifier == id;});
        if (PQPK_B_iterator == this->pqkemBulkPrekeys.end())
            throw std::runtime_error("Unknown one-time pq prekey ID");
        EVP_PKEY_ML_KEM_1024_ptr& PQPK_B = PQPK_B_iterator->key;
        SecretData<32> SS = decapsulateWithML_KEM_1024(PQPK_B, message.PQKEMCiphertext);
        SecretData<32> SK = generateHKDFKey(concatinateKDFInputs(DH_1, DH_2, DH_3, DH_4, SS).span());
        SS.wipe();
        std::vector<uint8_t> AD = generateAD(IK_A.getBytes(), IK_B.getPublicBytes(), PQPK_B);
        decryptWithAES_256_GCM(SK, message.initialCiphertext, AD);

        this->pqkemBulkPrekeys.erase(PQPK_B_iterator);
        this->curveBulkPrekeys.erase(OPK_B_iterator);

        return SK;
    }

}