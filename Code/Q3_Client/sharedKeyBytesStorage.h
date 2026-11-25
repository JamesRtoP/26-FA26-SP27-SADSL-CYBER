#ifndef SHAREDKEYBYTESSTORAGE_H
#define SHAREDKEYBYTESSTORAGE_H

#include "cryptography_primatives.h"
#include <QJsonObject>

namespace Q3Crypto
{
    class SharedKeyBytesStorage
    {
    private:
        identityKeyPublicBytes identity_key;
        prekeyWithIdentifierSignedBytesX25519 curvePrekey;
        prekeyWithIdentifierSignedBytesML_KEM_1024 lastResortpqkemKey;
        std::vector<prekeyWithIdentifierBytesX25519> curveBulkPrekeys;
        std::vector<prekeyWithIdentifierSignedBytesML_KEM_1024> pqkemBulkPrekeys;
    public:
        SharedKeyBytesStorage(const QJsonObject& json);
        SharedKeyBytesStorage(identityKeyPublicBytes new_identity_keyBytes,
                         prekeyWithIdentifierSignedBytesX25519 new_curvePrekeyBytes,prekeyWithIdentifierSignedBytesML_KEM_1024 new_lastResortpqkemKeyBytes,
                         std::vector<prekeyWithIdentifierBytesX25519> new_curveBulkPrekeysBytes, std::vector<prekeyWithIdentifierSignedBytesML_KEM_1024> new_pqkemBulkPrekeysBytes);

        bool checkSignatures ();
        QJsonObject toJson() const;

    };
}

#endif // SHAREDKEYBYTESSTORAGE_H
// identityKey identity_key, prekeyWithIdentifier curvePrekey, prekeyWithIdentifier lastResortpqkemKey, std::vector<prekeyWithIdentifier> curveBulkPrekeys, std::vector<prekeyWithIdentifier> pqkemBulkPrekeys