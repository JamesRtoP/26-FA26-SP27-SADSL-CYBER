#include "sharedKeyBytesStorage.h"
#include "identitykeypublic.h"
#include <qjsonarray.h>


namespace Q3Crypto
{
    SharedKeyBytesStorage::SharedKeyBytesStorage(const QJsonObject& json)
    {
        QJsonValue version = json.value("StorageType");
        if (!version.isString() || version.toString() != "SharedKeyBytesStorage")
            throw std::runtime_error("JSON: 'version' is missing or unsupported");

        QJsonValue identityValue = json.value("identityKey");
        if (!identityValue.isObject())
            throw std::runtime_error("JSON: 'identityKey' is missing or not an object");
        QJsonObject identity = identityValue.toObject();
        base64FieldToBytes(identity, "x25519", this->identity_key.X25519_key.data(), this->identity_key.X25519_key.size());
        base64FieldToBytes(identity, "ed25519", this->identity_key.Ed25519_key.data(), this->identity_key.Ed25519_key.size());

        QJsonValue signedPrekeyValue = json.value("signedPrekey");
        if (!signedPrekeyValue.isObject())
            throw std::runtime_error("JSON: 'signedPrekey' is missing or not an object");
        this->curvePrekey = curvePrekeySignedFromJson(signedPrekeyValue.toObject());

        QJsonValue lastResortValue = json.value("lastResortPqkemPrekey");
        if (!lastResortValue.isObject())
            throw std::runtime_error("JSON: 'lastResortPqkemPrekey' is missing or not an object");
        this->lastResortpqkemKey = pqkemPrekeySignedFromJson(lastResortValue.toObject());

        QJsonValue curveArrayValue = json.value("oneTimeCurvePrekeys");
        if (!curveArrayValue.isArray())
            throw std::runtime_error("JSON: 'oneTimeCurvePrekeys' is missing or not an array");
        QJsonArray curveArray = curveArrayValue.toArray();
        this->curveBulkPrekeys.reserve(curveArray.size());
        for (const QJsonValue& entry : curveArray)
        {
            if (!entry.isObject())
                throw std::runtime_error("JSON: 'oneTimeCurvePrekeys' entry is not an object");
            this->curveBulkPrekeys.push_back(curvePrekeyFromJson(entry.toObject()));
        }

        QJsonValue pqkemArrayValue = json.value("oneTimePqkemPrekeys");
        if (!pqkemArrayValue.isArray())
            throw std::runtime_error("JSON: 'oneTimePqkemPrekeys' is missing or not an array");
        QJsonArray pqkemArray = pqkemArrayValue.toArray();
        this->pqkemBulkPrekeys.reserve(pqkemArray.size());
        for (const QJsonValue& entry : pqkemArray)
        {
            if (!entry.isObject())
                throw std::runtime_error("JSON: 'oneTimePqkemPrekeys' entry is not an object");
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
        QJsonObject identity;
        identity["x25519"] = bytesToBase64(this->identity_key.X25519_key.data(), this->identity_key.X25519_key.size());
        identity["ed25519"] = bytesToBase64(this->identity_key.Ed25519_key.data(), this->identity_key.Ed25519_key.size());

        QJsonArray curveArray;
        for (const prekeyWithIdentifierBytesX25519& prekey : this->curveBulkPrekeys)
            curveArray.append(curvePrekeyToJson(prekey));

        QJsonArray pqkemArray;
        for (const prekeyWithIdentifierSignedBytesML_KEM_1024& prekey : this->pqkemBulkPrekeys)
            pqkemArray.append(pqkemPrekeySignedToJson(prekey));

        QJsonObject json;
        json["StorageType"] = "SharedKeyBytesStorage";
        json["identityKey"] = identity;
        json["signedPrekey"] = curvePrekeySignedToJson(this->curvePrekey);
        json["lastResortPqkemPrekey"] = pqkemPrekeySignedToJson(this->lastResortpqkemKey);
        json["oneTimeCurvePrekeys"] = curveArray;
        json["oneTimePqkemPrekeys"] = pqkemArray;
        return json;
    }

}