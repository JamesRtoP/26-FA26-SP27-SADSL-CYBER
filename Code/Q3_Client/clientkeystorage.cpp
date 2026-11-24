#include "clientkeystorage.h"
#include "sharedKeyBytesStorage.h"

namespace Q3Crypto
{

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
        std::vector<prekeyWithIdentifierSignedBytesX25519> new_curveBulkPrekeys = this->identity_key.signBulkPublicPrekeysX25519(this->curveBulkPrekeys);
        std::vector<prekeyWithIdentifierSignedBytesML_KEM_1024> new_pqkemBulkPrekeys = this->identity_key.signBulkPublicPrekeysML_KEM_1024(this->pqkemBulkPrekeys);
        return SharedKeyBytesStorage (IDKeyBytes, new_curvePrekey, new_lastResortpqkemKey, new_curveBulkPrekeys, new_pqkemBulkPrekeys);

    }

    IdentityKeyPair& ClientKeyStorage::getIdentityKey()
    {
        return this->identity_key;
    }


}