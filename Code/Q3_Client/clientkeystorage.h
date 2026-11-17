#ifndef CLIENTKEYSTORAGE_H
#define CLIENTKEYSTORAGE_H

#include "cryptography_primatives.h"
#include "identitykeypair.h"
#include "sharedKeyBytesStorage.h"
namespace Q3Crypto
{
    class ClientKeyStorage
    {
    private:
        IdentityKeyPair identity_key;
        prekeyWithIdentifierX25519 curvePrekey;
        prekeyWithIdentifierML_KEM_1024 lastResortpqkemKey;
        int nextCurveBulkPrekeysIdentifier;
        std::vector<prekeyWithIdentifierX25519> curveBulkPrekeys;
        int nextpqkemBulkPrekeysIdentifier;
        std::vector<prekeyWithIdentifierML_KEM_1024> pqkemBulkPrekeys;

    public:
        ClientKeyStorage();
        //ClientKeyStorage(identityKeyPairBytes identity_key_bytes, );

        //SharedKeyBytesStorage getKeyBytes();
    };
}
#endif // CLIENTKEYSTORAGE_H
