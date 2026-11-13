#ifndef SERVERKEYSTORAGE_H
#define SERVERKEYSTORAGE_H

#include "cryptography_primatives.h"

namespace Q3Crypto
{
    class ServerKeyStorage
    {
    private:
        identityKey identity_key;
        prekeyWithIdentifier curvePrekey;
        prekeyWithIdentifier lastResortpqkemKey;
        int nextCurveBulkPrekeysIdentifier;
        std::vector<prekeyWithIdentifier> curveBulkPrekeys;
        int nextpqkemBulkPrekeysIdentifier;
        std::vector<prekeyWithIdentifier> pqkemBulkPrekeys;
    public:
        ServerKeyStorage();
    };
}

#endif // SERVERKEYSTORAGE_H
