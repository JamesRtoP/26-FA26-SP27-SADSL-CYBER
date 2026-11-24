#include "sharedKeyBytesStorage.h"
#include "identitykeypublic.h".h"

namespace Q3Crypto
{

    SharedKeyBytesStorage::SharedKeyBytesStorage(identityKeyPublicBytes new_identity_key,
                                       prekeyWithIdentifierSignedBytesX25519 new_curvePrekey,prekeyWithIdentifierSignedBytesML_KEM_1024 new_lastResortpqkemKey,
                                       std::vector<prekeyWithIdentifierSignedBytesX25519> new_curveBulkPrekeys, std::vector<prekeyWithIdentifierSignedBytesML_KEM_1024> new_pqkemBulkPrekeys)
    {
        this->identity_key = new_identity_key;
        this->curvePrekey = new_curvePrekey;
        this->lastResortpqkemKey = new_lastResortpqkemKey;
        this->curveBulkPrekeys = new_curveBulkPrekeys;
        this->pqkemBulkPrekeys = new_pqkemBulkPrekeys;
    }

    bool SharedKeyBytesStorage::checkSignatures ()
    {
        IdentityKeyPublic checker(this->identity_key);
        return checker.checkSignature(this->curveBulkPrekeys) &&
                checker.checkSignature(this->pqkemBulkPrekeys) &&
               checker.checkSignature(this->curvePrekey) &&
               checker.checkSignature(this->lastResortpqkemKey);
    }

}