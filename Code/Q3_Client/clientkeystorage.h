#ifndef CLIENTKEYSTORAGE_H
#define CLIENTKEYSTORAGE_H

#include "cryptography_primatives.h"
namespace Q3Crypto
{
    class ClientKeyStorage
    {
    private:
        identityKey identity_key;
        int nextCurvePrekeyIdentifier;
        prekeyWithIdentifier curvePrekey;
        int nextpqkemPrekeyIdentifier;
        prekeyWithIdentifier lastResortpqkemKey;
        int nextCurveBulkPrekeysIdentifier;
        std::vector<prekeyWithIdentifier> curveBulkPrekeys;
        int nextpqkemBulkPrekeysIdentifier;
        std::vector<prekeyWithIdentifier> pqkemBulkPrekeys;

    public:
        ClientKeyStorage();
    };
}
#endif // CLIENTKEYSTORAGE_H
