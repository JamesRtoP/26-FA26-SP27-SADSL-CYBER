#include "sharedKeyBytesStorage.h"

namespace Q3Crypto
{

    SharedKeyBytesStorage::SharedKeyBytesStorage(identityKeyPairBytes new_identity_key,
                                       prekeyWithIdentifierSignedBytesX25519 new_curvePrekey,prekeyWithIdentifierSignedBytesML_KEM_1024 new_lastResortpqkemKey,
                                       std::vector<prekeyWithIdentifierSignedBytesX25519> new_curveBulkPrekeys, std::vector<prekeyWithIdentifierSignedBytesML_KEM_1024> new_pqkemBulkPrekeys)
    {
        this->identity_key = new_identity_key;
        this->curvePrekey = new_curvePrekey;
        this->lastResortpqkemKey = new_lastResortpqkemKey;
        this->curveBulkPrekeys = new_curveBulkPrekeys;
        this->pqkemBulkPrekeys = new_pqkemBulkPrekeys;
    }

}