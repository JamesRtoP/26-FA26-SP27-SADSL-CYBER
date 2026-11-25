#include "sharedKeyBytesStorage.h"
#include "identitykeypublic.h"
#include <qjsonarray.h>


namespace Q3Crypto
{
SharedKeyBytesStorage::SharedKeyBytesStorage(const QJsonObject& json)
{
    QJsonValue storageType = json.value("StorageType");
    if (!storageType.isString() || storageType.toString() != "SharedKeyBytesStorage")
        throw std::runtime_error("JSON: 'StorageType' is missing or not SharedKeyBytesStorage");

    base64FieldToBytes(json, "identityKey",
                       this->identity_key.X25519_key.data(), this->identity_key.X25519_key.size());
    base64FieldToBytes(json, "identitySigningKey",
                       this->identity_key.Ed25519_key.data(), this->identity_key.Ed25519_key.size());

    QJsonValue signedPreKeyValue = json.value("signedPreKey");
    if (!signedPreKeyValue.isObject())
        throw std::runtime_error("JSON: 'signedPreKey' is missing or not an object");
    this->curvePrekey = curvePrekeySignedFromJson(signedPreKeyValue.toObject());

    QJsonValue pqSignedPreKeyValue = json.value("pqSignedPreKey");
    if (!pqSignedPreKeyValue.isObject())
        throw std::runtime_error("JSON: 'pqSignedPreKey' is missing or not an object");
    this->lastResortpqkemKey = pqkemPrekeySignedFromJson(pqSignedPreKeyValue.toObject());

    QJsonValue curveArrayValue = json.value("oneTimePreKeys");
    if (!curveArrayValue.isArray())
        throw std::runtime_error("JSON: 'oneTimePreKeys' is missing or not an array");
    const QJsonArray curveArray = curveArrayValue.toArray();
    this->curveBulkPrekeys.reserve(curveArray.size());
    for (const QJsonValue& entry : curveArray)
    {
        if (!entry.isObject())
            throw std::runtime_error("JSON: 'oneTimePreKeys' entry is not an object");
        this->curveBulkPrekeys.push_back(curvePrekeyFromJson(entry.toObject()));
    }

    QJsonValue pqkemArrayValue = json.value("pqOneTimePreKeys");
    if (!pqkemArrayValue.isArray())
        throw std::runtime_error("JSON: 'pqOneTimePreKeys' is missing or not an array");
    const QJsonArray pqkemArray = pqkemArrayValue.toArray();
    this->pqkemBulkPrekeys.reserve(pqkemArray.size());
    for (const QJsonValue& entry : pqkemArray)
    {
        if (!entry.isObject())
            throw std::runtime_error("JSON: 'pqOneTimePreKeys' entry is not an object");
        this->pqkemBulkPrekeys.push_back(pqkemPrekeySignedFromJson(entry.toObject()));
    }
}

    SharedKeyBytesStorage::SharedKeyBytesStorage(identityKeyPublicBytes new_identity_key,
                                       prekeyWithIdentifierSignedBytesX25519 new_curvePrekey,prekeyWithIdentifierSignedBytesML_KEM_1024 new_lastResortpqkemKey,
                                       std::vector<prekeyWithIdentifierBytesX25519> new_curveBulkPrekeys, std::vector<prekeyWithIdentifierSignedBytesML_KEM_1024> new_pqkemBulkPrekeys)
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
        return checker.checkSignature(this->pqkemBulkPrekeys) &&
               checker.checkSignature(this->curvePrekey) &&
               checker.checkSignature(this->lastResortpqkemKey);
    }

    QJsonObject SharedKeyBytesStorage::toJson() const
    {

        QJsonArray curveArray;
        for (const prekeyWithIdentifierBytesX25519& prekey : this->curveBulkPrekeys)
            curveArray.append(curvePrekeyToJson(prekey));

        QJsonArray pqkemArray;
        for (const prekeyWithIdentifierSignedBytesML_KEM_1024& prekey : this->pqkemBulkPrekeys)
            pqkemArray.append(pqkemPrekeySignedToJson(prekey));

        QJsonObject json;
        json["StorageType"] = "SharedKeyBytesStorage";
        json["identityKey"] = bytesToBase64(this->identity_key.X25519_key.data(), this->identity_key.X25519_key.size());;
        json["identitySigningKey"] = bytesToBase64(this->identity_key.Ed25519_key.data(), this->identity_key.Ed25519_key.size());
        json["signedPreKey"] = curvePrekeySignedToJson(this->curvePrekey);
        json["pqSignedPreKey"] = pqkemPrekeySignedToJson(this->lastResortpqkemKey);
        json["oneTimePreKeys"] = curveArray;
        json["pqOneTimePreKeys"] = pqkemArray;
        return json;
    }

}