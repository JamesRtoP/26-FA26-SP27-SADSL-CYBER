#include "clientkeystorage.h"

namespace Q3Crypto
{

    ClientKeyStorage::ClientKeyStorage()
    {
        this->identity_key = generateIdentityKey();
        this->nextCurvePrekeyIdentifier = 0;
        this->curvePrekey = {generateX25519Key(), this->nextCurvePrekeyIdentifier};

        this->nextpqkemPrekeyIdentifier = 100;
        this->lastResortpqkemKey = {generateML_KEM_1024Key(),this->nextpqkemPrekeyIdentifier++};


        this->nextCurvePrekeysID = 10000;
        this->curvePrekeys = generatePreKeysX25519(50, this->nextCurvePrekeysID);

        this->nextpqkemPrekeysID = 20000;
        this->pqkemPrekeys = generatePreKeysML_KEM_1024(50, this->nextpqkemPrekeysID = 20000);
    }

}