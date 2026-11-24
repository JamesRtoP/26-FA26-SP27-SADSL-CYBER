// HTTP routes for PQXDH public key bundles. The server stores and hands out public keys
// only; it never sees private keys and never verifies signatures (recipients do).
#include "keyroutes.h"
#include "database.h"

#include <QHttpServer>
#include <QJsonArray>
#include <QJsonDocument>

namespace {

// Large enough for any ML-KEM-1024 key in DER form, small enough to bound a request.
constexpr qsizetype MaxKeyBytes = 8192;
// Upper bound on one-time prekeys per kind in a single upload.
constexpr qsizetype MaxPreKeysPerUpload = 100;
constexpr qint64 MaxKeyId = 4294967295; // Key IDs are uint32 on the client.

/**
 * @brief Resolve the username/authToken pair in a request body to its user.
 * @param database Open database.
 * @param body Parsed request body.
 * @return The authenticated user ID, or std::nullopt if the token is invalid or belongs to someone else.
 */
std::optional<qint64> authenticate(Database &database, const QJsonObject &body)
{
    const std::optional<qint64> sessionUserId =
        database.sessionUser(body.value("authToken").toString().toUtf8());
    const std::optional<StoredUser> user = database.user(body.value("username").toString());
    if (!sessionUserId || !user || *sessionUserId != user->id)
        return std::nullopt;
    return user->id;
}

/**
 * @brief Decode a required base64 field holding key material.
 * @param object JSON object containing the field.
 * @param name Field name.
 * @return The decoded bytes, or std::nullopt if missing, not strict base64, empty, or too large.
 */
std::optional<QByteArray> keyBytes(const QJsonObject &object, const char *name)
{
    const QJsonValue value = object.value(QLatin1String(name));
    if (!value.isString())
        return std::nullopt;
    const auto decoded = QByteArray::fromBase64Encoding(value.toString().toLatin1(),
                                                        QByteArray::AbortOnBase64DecodingErrors);
    if (!decoded || decoded.decoded.isEmpty() || decoded.decoded.size() > MaxKeyBytes)
        return std::nullopt;
    return decoded.decoded;
}

/**
 * @brief Parse a prekey object of the form {"id", "publicKey"[, "signature"]}.
 * @param value JSON value to parse.
 * @param signed_ Whether a "signature" field is required.
 * @return The prekey, or std::nullopt if any field is missing or invalid.
 */
std::optional<OneTimePreKey> preKey(const QJsonValue &value, bool signed_)
{
    const QJsonObject object = value.toObject();
    const QJsonValue id = object.value("id");
    const qint64 keyId = id.toInteger(-1);
    // Reject fractions and out-of-range IDs rather than silently truncating them.
    if (!id.isDouble() || keyId < 0 || keyId > MaxKeyId || double(keyId) != id.toDouble())
        return std::nullopt;
    const std::optional<QByteArray> publicKey = keyBytes(object, "publicKey");
    if (!publicKey)
        return std::nullopt;
    OneTimePreKey key{keyId, *publicKey, {}};
    if (signed_) {
        const std::optional<QByteArray> signature = keyBytes(object, "signature");
        if (!signature)
            return std::nullopt;
        key.signature = *signature;
    }
    return key;
}

/**
 * @brief Parse an optional array of one-time prekeys.
 * @param keys The uploaded "keys" object.
 * @param name Array field name; a missing field is an empty list.
 * @param signed_ Whether each prekey needs a signature.
 * @return The prekeys, or std::nullopt if the field is malformed or too long.
 */
std::optional<QList<OneTimePreKey>> preKeyList(const QJsonObject &keys, const char *name, bool signed_)
{
    const QJsonValue value = keys.value(QLatin1String(name));
    if (value.isUndefined())
        return QList<OneTimePreKey>{};
    if (!value.isArray() || value.toArray().size() > MaxPreKeysPerUpload)
        return std::nullopt;
    QList<OneTimePreKey> result;
    for (const QJsonValue &item : value.toArray()) {
        const std::optional<OneTimePreKey> key = preKey(item, signed_);
        if (!key)
            return std::nullopt;
        result.append(*key);
    }
    return result;
}

/**
 * @brief Serialize a prekey for a response, in the same shape clients upload.
 * @param key Prekey to serialize.
 * @return {"id", "publicKey"[, "signature"]} with base64 bytes.
 */
QJsonObject preKeyJson(const OneTimePreKey &key)
{
    QJsonObject object{{"id", key.keyId},
                       {"publicKey", QString::fromLatin1(key.publicKey.toBase64())}};
    if (!key.signature.isEmpty())
        object["signature"] = QString::fromLatin1(key.signature.toBase64());
    return object;
}

QHttpServerResponse text(const char *message, QHttpServerResponse::StatusCode status)
{
    return QHttpServerResponse(QString::fromLatin1(message), status);
}

} // namespace

void addKeyRoutes(QHttpServer &server, Database &database)
{
    using Status = QHttpServerResponse::StatusCode;

    // Publish the caller's own bundle and top up their one-time prekey pools.
    server.route("/user/keys", QHttpServerRequest::Method::Post,
                 [&database](const QHttpServerRequest &request) {
        const QJsonDocument document = QJsonDocument::fromJson(request.body());
        if (!document.isObject())
            return text("Invalid JSON payload", Status::BadRequest);
        const QJsonObject body = document.object();
        const std::optional<qint64> userId = authenticate(database, body);
        if (!userId)
            return text("Authentication Mismatch", Status::Unauthorized);

        const QJsonObject keys = body.value("keys").toObject();
        const std::optional<QByteArray> identityKey = keyBytes(keys, "identityKey");
        const std::optional<OneTimePreKey> signedPreKey = preKey(keys.value("signedPreKey"), true);
        const std::optional<OneTimePreKey> pqSignedPreKey = preKey(keys.value("pqSignedPreKey"), true);
        const std::optional<QList<OneTimePreKey>> curvePreKeys = preKeyList(keys, "oneTimePreKeys", false);
        const std::optional<QList<OneTimePreKey>> pqPreKeys = preKeyList(keys, "pqOneTimePreKeys", true);
        if (!identityKey || !signedPreKey || !pqSignedPreKey || !curvePreKeys || !pqPreKeys)
            return text("Invalid key bundle", Status::BadRequest);

        // Store only recognized fields, re-encoded, so every bundle has the documented shape.
        QJsonObject bundle{{"identityKey", QString::fromLatin1(identityKey->toBase64())},
                           {"signedPreKey", preKeyJson(*signedPreKey)},
                           {"pqSignedPreKey", preKeyJson(*pqSignedPreKey)}};
        // The Ed25519 half of a split identity key, when the client publishes one.
        if (keys.contains("identitySigningKey")) {
            const std::optional<QByteArray> signingKey = keyBytes(keys, "identitySigningKey");
            if (!signingKey)
                return text("Invalid key bundle", Status::BadRequest);
            bundle["identitySigningKey"] = QString::fromLatin1(signingKey->toBase64());
        }

        const std::optional<PreKeyCounts> counts =
            database.storeKeyBundle(*userId, bundle, *curvePreKeys, *pqPreKeys);
        if (!counts) {
            qWarning() << "Failed to store key bundle:" << database.lastError();
            return text("Failed to store keys", Status::InternalServerError);
        }
        // Remaining pool sizes tell the client when to upload more one-time prekeys.
        return QHttpServerResponse(QJsonObject{{"oneTimePreKeys", counts->curve},
                                               {"pqOneTimePreKeys", counts->postQuantum}},
                                   Status::Ok);
    });

    // Fetch a recipient's bundle to start a PQXDH handshake. Each call consumes up to
    // one prekey from each of the recipient's one-time pools.
    server.route("/user/keys", QHttpServerRequest::Method::Get,
                 [&database](const QHttpServerRequest &request) {
        const QJsonDocument document = QJsonDocument::fromJson(request.body());
        if (!document.isObject())
            return text("Invalid JSON payload", Status::BadRequest);
        const QJsonObject body = document.object();
        if (!authenticate(database, body))
            return text("Authentication Mismatch", Status::Unauthorized);

        const std::optional<StoredUser> recipient = database.user(body.value("recipient").toString());
        if (!recipient) {
            if (!database.lastError().isEmpty()) {
                qWarning() << "Failed to look up recipient:" << database.lastError();
                return text("Failed to retrieve keys", Status::InternalServerError);
            }
            return text("Recipient Does Not Exist", Status::NotFound);
        }

        const std::optional<ClaimedKeyBundle> claimed = database.claimKeyBundle(recipient->id);
        if (!claimed) {
            if (!database.lastError().isEmpty()) {
                qWarning() << "Failed to claim key bundle:" << database.lastError();
                return text("Failed to retrieve keys", Status::InternalServerError);
            }
            return text("Recipient Has No Keys", Status::NotFound);
        }

        QJsonObject keys = claimed->bundle;
        if (claimed->curvePreKey)
            keys["oneTimePreKey"] = preKeyJson(*claimed->curvePreKey);
        if (claimed->pqPreKey)
            keys["pqOneTimePreKey"] = preKeyJson(*claimed->pqPreKey);
        return QHttpServerResponse(QJsonObject{{"keys", keys}}, Status::Ok);
    });
}
