#include "keybundlejson.h"

#include <QJsonArray>
#include <climits>
#include <cstring>

namespace Q3Crypto
{
    namespace
    {
        template <std::size_t N>
        QString toBase64(const std::array<std::uint8_t, N> &bytes)
        {
            return QString::fromLatin1(QByteArray(reinterpret_cast<const char *>(bytes.data()), N).toBase64());
        }

        // Decode a base64 field into a fixed-size array. Fails unless the field exists, is strict
        // base64, and decodes to exactly N bytes, because the receiving structs have fixed sizes.
        template <std::size_t N>
        bool fromBase64(const QJsonObject &object, const char *name, std::array<std::uint8_t, N> &out)
        {
            const QJsonValue value = object.value(QLatin1String(name));
            if (!value.isString())
                return false;
            const auto decoded = QByteArray::fromBase64Encoding(value.toString().toLatin1(),
                                                                QByteArray::AbortOnBase64DecodingErrors);
            if (!decoded || decoded.decoded.size() != qsizetype(N))
                return false;
            std::memcpy(out.data(), decoded.decoded.constData(), N);
            return true;
        }

        // The structs store ids as int, so accept whole numbers from 0 to INT_MAX only.
        bool readId(const QJsonObject &object, int &out)
        {
            const QJsonValue value = object.value(QLatin1String("id"));
            if (!value.isDouble())
                return false;
            const double id = value.toDouble();
            if (id < 0 || id > INT_MAX || id != double(qint64(id)))
                return false;
            out = int(id);
            return true;
        }

        template <typename Signed>
        QJsonObject signedJson(const Signed &key)
        {
            return QJsonObject{{"id", key.identifier},
                               {"publicKey", toBase64(key.key)},
                               {"signature", toBase64(key.signature)}};
        }

        template <typename Signed>
        bool readSigned(const QJsonValue &value, Signed &out)
        {
            const QJsonObject object = value.toObject();
            return value.isObject() && readId(object, out.identifier)
                   && fromBase64(object, "publicKey", out.key) && fromBase64(object, "signature", out.signature);
        }
    }

    QJsonObject toUploadJson(const SharedKeyBytesStorage &keys)
    {
        QJsonArray curve;
        for (const auto &key : keys.getCurveBulkPrekeys())
            curve.append(QJsonObject{{"id", key.identifier}, {"publicKey", toBase64(key.key)}});
        QJsonArray pq;
        for (const auto &key : keys.getPqkemBulkPrekeys())
            pq.append(signedJson(key));

        return QJsonObject{
            {"identityKey", toBase64(keys.getIdentityKey().X25519_key)},
            {"identitySigningKey", toBase64(keys.getIdentityKey().Ed25519_key)},
            {"signedPreKey", signedJson(keys.getCurvePrekey())},
            {"pqSignedPreKey", signedJson(keys.getLastResortPqkemKey())},
            {"oneTimePreKeys", curve},
            {"pqOneTimePreKeys", pq},
        };
    }

    std::optional<ReceivedKeyBundle> fromKeysJson(const QJsonObject &keys)
    {
        ReceivedKeyBundle bundle{};
        // The Ed25519 half is required: without it the prekey signatures can't be checked.
        if (!fromBase64(keys, "identityKey", bundle.identityKey.X25519_key)
            || !fromBase64(keys, "identitySigningKey", bundle.identityKey.Ed25519_key)
            || !readSigned(keys.value("signedPreKey"), bundle.signedPreKey)
            || !readSigned(keys.value("pqSignedPreKey"), bundle.pqSignedPreKey))
            return std::nullopt;

        // One-time prekeys are optional, but if present they must be valid.
        if (keys.contains("oneTimePreKey")) {
            const QJsonObject object = keys.value("oneTimePreKey").toObject();
            prekeyWithIdentifierBytesX25519 key{};
            if (!readId(object, key.identifier) || !fromBase64(object, "publicKey", key.key))
                return std::nullopt;
            bundle.oneTimePreKey = key;
        }
        if (keys.contains("pqOneTimePreKey")) {
            prekeyWithIdentifierSignedBytesML_KEM_1024 key{};
            if (!readSigned(keys.value("pqOneTimePreKey"), key))
                return std::nullopt;
            bundle.pqOneTimePreKey = key;
        }
        return bundle;
    }
}
