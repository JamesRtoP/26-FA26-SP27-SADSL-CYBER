#include "clientkeystorage.h"
#include "sharedKeyBytesStorage.h"

namespace Q3Crypto
{

    ClientKeyStorage::ClientKeyStorage()
    {
        this->identity_key = generateIdentityKey();
        this->nextCurvePrekeyIdentifier = 0;
        this->curvePrekey = {generateX25519Key(), this->nextCurvePrekeyIdentifier};

        this->nextpqkemPrekeyIdentifier = 100;
        this->lastResortpqkemKey = {generateML_KEM_1024Key(),this->nextpqkemPrekeyIdentifier++};


        this->nextCurveBulkPrekeysIdentifier = 10000;
        this->curveBulkPrekeys = generatePreKeysX25519(50, this->nextCurveBulkPrekeysIdentifier);

        this->nextpqkemBulkPrekeysIdentifier = 20000;
        this->pqkemBulkPrekeys = generatePreKeysML_KEM_1024(50, this->nextpqkemBulkPrekeysIdentifier = 20000);
    }

    SharedKeyBytesStorage getKeyBytes()
    {
        identityKeyBytes IDKeyBytes;
        prekeyBytesWithIdentifierSigned new_curvePrekey;
        prekeyBytesWithIdentifierSigned new_lastResortpqkemKey;
        std::vector<prekeyBytesWithIdentifierSigned> new_curveBulkPrekeys;
        std::vector<prekeyBytesWithIdentifierSigned> new_pqkemBulkPrekeys;
        SharedKeyBytesStorage sharedBytes();

    }


}