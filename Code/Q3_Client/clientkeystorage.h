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
        int nextCurvePrekeysID;
        std::vector<prekeyWithIdentifier> curvePrekeys;
        int nextpqkemPrekeysID;
        std::vector<prekeyWithIdentifier> pqkemPrekeys;

    public:
        ClientKeyStorage();
    };
}
#endif // CLIENTKEYSTORAGE_H
