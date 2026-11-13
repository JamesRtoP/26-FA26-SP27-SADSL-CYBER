#ifndef SHAREDKEYBYTESSTORAGE_H
#define SHAREDKEYBYTESSTORAGE_H

#include "cryptography_primatives.h"

namespace Q3Crypto
{
    class SharedKeyBytesStorage
    {
    private:
        identityKey identity_key;
        prekeyBytesWithIdentifierSigned curvePrekey;
        prekeyBytesWithIdentifierSigned lastResortpqkemKey;
        std::vector<prekeyBytesWithIdentifierSigned> curveBulkPrekeys;
        std::vector<prekeyBytesWithIdentifierSigned> pqkemBulkPrekeys;
    public:
        SharedKeyBytesStorage(identityKey new_identity_key,
                         prekeyBytesWithIdentifierSigned new_curvePrekey,prekeyBytesWithIdentifierSigned new_lastResortpqkemKey,
                         std::vector<prekeyBytesWithIdentifierSigned> new_curveBulkPrekeys, std::vector<prekeyBytesWithIdentifierSigned> new_pqkemBulkPrekeys);
    };
}

#endif // SHAREDKEYBYTESSTORAGE_H
