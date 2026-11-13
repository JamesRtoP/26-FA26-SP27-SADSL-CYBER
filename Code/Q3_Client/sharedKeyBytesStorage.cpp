#include "sharedKeyBytesStorage.h"

namespace Q3Crypto
{

    SharedKeyBytesStorage::SharedKeyBytesStorage(identityKeyBytes new_identity_key,
                                       prekeyBytesWithIdentifierSigned new_curvePrekey,prekeyBytesWithIdentifierSigned new_lastResortpqkemKey,
                                       std::vector<prekeyBytesWithIdentifierSigned> new_curveBulkPrekeys, std::vector<prekeyBytesWithIdentifierSigned> new_pqkemBulkPrekeys)
    {
        this->identity_key = new_identity_key;
        this->curvePrekey = new_curvePrekey;
        this->lastResortpqkemKey = new_lastResortpqkemKey;
        this->curveBulkPrekeys = new_curveBulkPrekeys;
        this->pqkemBulkPrekeys = new_pqkemBulkPrekeys;
    }

    SharedKeyBytesStorage::SharedKeyBytesStorage(identityKey new_identity_key,
                          prekeyWithIdentifier new_curvePrekey, prekeyWithIdentifier new_lastResortpqkemKey,
                          std::vector<prekeyWithIdentifier> new_curveBulkPrekeys, std::vector<prekeyWithIdentifier> new_pqkemBulkPrekeys)
    {
        this->identity_key = generateIdentityKeyBytes(new_identity_key);
        this->curvePrekey = signPublicPrekeyX25519(new_identity_key, new_curvePrekey);
        this->lastResortpqkemKey = signPublicPrekeyML_KEM_1024(new_identity_key, new_lastResortpqkemKey);
        this->curveBulkPrekeys = new_curveBulkPrekeys;
        this->pqkemBulkPrekeys = new_pqkemBulkPrekeys;
    }
}