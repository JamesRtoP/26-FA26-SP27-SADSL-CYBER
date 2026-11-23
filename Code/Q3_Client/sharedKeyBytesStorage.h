#ifndef SHAREDKEYBYTESSTORAGE_H
#define SHAREDKEYBYTESSTORAGE_H

#include "cryptography_primatives.h"

namespace Q3Crypto
{
    class SharedKeyBytesStorage
    {
    private:
        identityKeyPairBytes identity_key;
        prekeyWithIdentifierSignedBytesX25519 curvePrekey;
        prekeyWithIdentifierSignedBytesML_KEM_1024 lastResortpqkemKey;
        std::vector<prekeyWithIdentifierSignedBytesX25519> curveBulkPrekeys;
        std::vector<prekeyWithIdentifierSignedBytesML_KEM_1024> pqkemBulkPrekeys;
    public:
        SharedKeyBytesStorage(identityKeyPairBytes new_identity_keyBytes,
                         prekeyWithIdentifierSignedBytesX25519 new_curvePrekeyBytes,prekeyWithIdentifierSignedBytesML_KEM_1024 new_lastResortpqkemKeyBytes,
                         std::vector<prekeyWithIdentifierSignedBytesX25519> new_curveBulkPrekeysBytes, std::vector<prekeyWithIdentifierSignedBytesML_KEM_1024> new_pqkemBulkPrekeysBytes);

        // Read-only access so the keys can be converted for upload (see keybundlejson.h).
        const identityKeyPairBytes& getIdentityKey() const { return identity_key; }
        const prekeyWithIdentifierSignedBytesX25519& getCurvePrekey() const { return curvePrekey; }
        const prekeyWithIdentifierSignedBytesML_KEM_1024& getLastResortPqkemKey() const { return lastResortpqkemKey; }
        const std::vector<prekeyWithIdentifierSignedBytesX25519>& getCurveBulkPrekeys() const { return curveBulkPrekeys; }
        const std::vector<prekeyWithIdentifierSignedBytesML_KEM_1024>& getPqkemBulkPrekeys() const { return pqkemBulkPrekeys; }
    };
}

#endif // SHAREDKEYBYTESSTORAGE_H
// identityKey identity_key, prekeyWithIdentifier curvePrekey, prekeyWithIdentifier lastResortpqkemKey, std::vector<prekeyWithIdentifier> curveBulkPrekeys, std::vector<prekeyWithIdentifier> pqkemBulkPrekeys