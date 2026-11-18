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

    /*
    SharedKeyBytesStorage getKeyBytes()
    {
        identityKeyPairBytes IDKeyBytes;
        prekeyBytesWithIdentifierSigned new_curvePrekey;
        prekeyBytesWithIdentifierSigned new_lastResortpqkemKey;
        std::vector<prekeyBytesWithIdentifierSigned> new_curveBulkPrekeys;
        std::vector<prekeyBytesWithIdentifierSigned> new_pqkemBulkPrekeys;
        SharedKeyBytesStorage sharedBytes();

    }
    */

}