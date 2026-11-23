// Unit tests for keybundlejson: struct -> JSON -> struct must keep every byte, and malformed
// server replies must be rejected. Keys here are random bytes unless a test needs real keys,
// so these tests check the conversion only, not OpenSSL signing.
#include "identitykeypair.h"
#include "identitykeypublic.h"
#include "keybundlejson.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QRandomGenerator>
#include <QtTest>

using namespace Q3Crypto;

namespace
{
    template <std::size_t N>
    std::array<std::uint8_t, N> randomBytes()
    {
        std::array<std::uint8_t, N> out{};
        for (auto &b : out)
            b = std::uint8_t(QRandomGenerator::global()->bounded(256));
        return out;
    }

    template <typename T>
    T randomSigned(int id)
    {
        T key{};
        key.key = randomBytes<std::tuple_size_v<decltype(key.key)>>();
        key.signature = randomBytes<64>();
        key.identifier = id;
        return key;
    }

    SharedKeyBytesStorage randomStorage()
    {
        identityKeyPairBytes identity{randomBytes<33>(), randomBytes<33>()};
        std::vector<prekeyWithIdentifierSignedBytesX25519> curve;
        for (int i = 0; i < 3; ++i)
            curve.push_back(randomSigned<prekeyWithIdentifierSignedBytesX25519>(10000 + i));
        std::vector<prekeyWithIdentifierSignedBytesML_KEM_1024> pq;
        for (int i = 0; i < 2; ++i)
            pq.push_back(randomSigned<prekeyWithIdentifierSignedBytesML_KEM_1024>(20000 + i));
        return SharedKeyBytesStorage(identity, randomSigned<prekeyWithIdentifierSignedBytesX25519>(0),
                                     randomSigned<prekeyWithIdentifierSignedBytesML_KEM_1024>(100), curve, pq);
    }

    // What GET /user/keys sends back after an upload: the long-lived keys plus the lowest-id
    // one-time prekey from each pool (see claimKeyBundle on the server).
    QJsonObject serverReplyFor(const QJsonObject &upload)
    {
        QJsonObject reply{{"identityKey", upload["identityKey"]},
                          {"identitySigningKey", upload["identitySigningKey"]},
                          {"signedPreKey", upload["signedPreKey"]},
                          {"pqSignedPreKey", upload["pqSignedPreKey"]}};
        reply["oneTimePreKey"] = upload["oneTimePreKeys"].toArray().first();
        reply["pqOneTimePreKey"] = upload["pqOneTimePreKeys"].toArray().first();
        return reply;
    }

    QJsonObject validReply()
    {
        return serverReplyFor(toUploadJson(randomStorage()));
    }

    QString b64(int byteCount)
    {
        return QString::fromLatin1(QByteArray(byteCount, 'x').toBase64());
    }

    // Send one JSON request and wait for the reply (the server routes read JSON bodies, GET included).
    std::pair<int, QByteArray> call(QNetworkAccessManager &net, const QString &base, const QByteArray &verb,
                                    const QString &path, const QJsonObject &body)
    {
        QNetworkRequest request(QUrl(base + path));
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
        request.setTransferTimeout(10000);
        QNetworkReply *reply = net.sendCustomRequest(request, verb, QJsonDocument(body).toJson(QJsonDocument::Compact));
        QEventLoop loop;
        QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        loop.exec();
        const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        const QByteArray data = reply->readAll();
        reply->deleteLater();
        return {status, data};
    }
}

class TestKeyBundleJson : public QObject
{
    Q_OBJECT
private slots:
    void uploadHasTheServerFormat();
    void roundTripKeepsEveryByte();
    void oneTimeKeysAreOptional();
    void realKeysStillDecode();
    void rejectsBadReplies_data();
    void rejectsBadReplies();
    void endToEndThroughServer();
};

void TestKeyBundleJson::uploadHasTheServerFormat()
{
    const SharedKeyBytesStorage keys = randomStorage();
    const QJsonObject json = toUploadJson(keys);

    QCOMPARE(json.keys(), (QStringList{"identityKey", "identitySigningKey", "oneTimePreKeys",
                                       "pqOneTimePreKeys", "pqSignedPreKey", "signedPreKey"}));
    // 33 bytes -> 44 base64 characters; a 1569-byte ML-KEM key -> 2092.
    QCOMPARE(json["identityKey"].toString().size(), 44);
    QCOMPARE(json["identitySigningKey"].toString().size(), 44);
    QCOMPARE(json["pqSignedPreKey"]["publicKey"].toString().size(), 2092);
    QCOMPARE(json["signedPreKey"]["id"].toInt(), 0);
    QCOMPARE(json["pqSignedPreKey"]["id"].toInt(), 100);

    const QJsonArray curve = json["oneTimePreKeys"].toArray();
    QCOMPARE(curve.size(), 3);
    QCOMPARE(curve[0]["id"].toInt(), 10000);
    QVERIFY2(!curve[0].toObject().contains("signature"), "curve one-time prekeys are sent unsigned");
    const QJsonArray pq = json["pqOneTimePreKeys"].toArray();
    QCOMPARE(pq.size(), 2);
    QVERIFY(pq[0].toObject().contains("signature"));
}

void TestKeyBundleJson::roundTripKeepsEveryByte()
{
    const SharedKeyBytesStorage keys = randomStorage();
    const auto bundle = fromKeysJson(serverReplyFor(toUploadJson(keys)));
    QVERIFY(bundle);

    QCOMPARE(bundle->identityKey.X25519_key, keys.getIdentityKey().X25519_key);
    QCOMPARE(bundle->identityKey.Ed25519_key, keys.getIdentityKey().Ed25519_key);
    QCOMPARE(bundle->signedPreKey.key, keys.getCurvePrekey().key);
    QCOMPARE(bundle->signedPreKey.signature, keys.getCurvePrekey().signature);
    QCOMPARE(bundle->signedPreKey.identifier, keys.getCurvePrekey().identifier);
    QCOMPARE(bundle->pqSignedPreKey.key, keys.getLastResortPqkemKey().key);
    QCOMPARE(bundle->pqSignedPreKey.signature, keys.getLastResortPqkemKey().signature);
    QCOMPARE(bundle->pqSignedPreKey.identifier, keys.getLastResortPqkemKey().identifier);

    QVERIFY(bundle->oneTimePreKey);
    QCOMPARE(bundle->oneTimePreKey->key, keys.getCurveBulkPrekeys()[0].key);
    QCOMPARE(bundle->oneTimePreKey->identifier, keys.getCurveBulkPrekeys()[0].identifier);
    QVERIFY(bundle->pqOneTimePreKey);
    QCOMPARE(bundle->pqOneTimePreKey->key, keys.getPqkemBulkPrekeys()[0].key);
    QCOMPARE(bundle->pqOneTimePreKey->signature, keys.getPqkemBulkPrekeys()[0].signature);
    QCOMPARE(bundle->pqOneTimePreKey->identifier, keys.getPqkemBulkPrekeys()[0].identifier);
}

void TestKeyBundleJson::oneTimeKeysAreOptional()
{
    QJsonObject reply = validReply();
    reply.remove("oneTimePreKey");
    reply.remove("pqOneTimePreKey");
    const auto bundle = fromKeysJson(reply);
    QVERIFY(bundle);
    QVERIFY(!bundle->oneTimePreKey);
    QVERIFY(!bundle->pqOneTimePreKey);
}

// Real keys from Q3Crypto must survive the trip in a form the decodePublic* functions accept.
void TestKeyBundleJson::realKeysStillDecode()
{
    IdentityKeyPair identity;
    int curveId = 0, pqId = 100;
    auto curve = generatePreKeysX25519(1, curveId);
    auto pq = generatePreKeysML_KEM_1024(1, pqId);
    prekeyWithIdentifierSignedBytesX25519 spk{encodePublicX25519(curve[0].key), curve[0].identifier, randomBytes<64>()};
    prekeyWithIdentifierSignedBytesML_KEM_1024 pqspk{encodePublicML_KEM_1024(pq[0].key), pq[0].identifier, randomBytes<64>()};
    const SharedKeyBytesStorage keys(identity.getBytes(), spk, pqspk, {}, {});

    const auto bundle = fromKeysJson(toUploadJson(keys));
    QVERIFY(bundle);
    QVERIFY(decodePublicX25519(bundle->identityKey.X25519_key));
    QVERIFY(decodePublicEd25519(bundle->identityKey.Ed25519_key));
    QVERIFY(decodePublicX25519(bundle->signedPreKey.key));
    QVERIFY(decodePublicML_KEM_1024(bundle->pqSignedPreKey.key));
}

void TestKeyBundleJson::rejectsBadReplies_data()
{
    QTest::addColumn<QJsonObject>("reply");
    const QJsonObject good = validReply();
    auto with = [&](const char *field, const QJsonValue &value) { QJsonObject r = good; r[field] = value; return r; };
    auto without = [&](const char *field) { QJsonObject r = good; r.remove(field); return r; };
    auto inner = [&](const char *field, const char *key, const QJsonValue &value) {
        QJsonObject r = good; QJsonObject o = r[field].toObject(); o[key] = value; r[field] = o; return r; };
    auto innerWithout = [&](const char *field, const char *key) {
        QJsonObject r = good; QJsonObject o = r[field].toObject(); o.remove(key); r[field] = o; return r; };

    QTest::newRow("missing identityKey") << without("identityKey");
    QTest::newRow("missing identitySigningKey") << without("identitySigningKey");
    QTest::newRow("missing signedPreKey") << without("signedPreKey");
    QTest::newRow("missing pqSignedPreKey") << without("pqSignedPreKey");
    QTest::newRow("identityKey not base64") << with("identityKey", "not base64!!");
    QTest::newRow("identityKey 32 bytes") << with("identityKey", b64(32));
    QTest::newRow("identityKey 34 bytes") << with("identityKey", b64(34));
    QTest::newRow("identityKey is a number") << with("identityKey", 5);
    QTest::newRow("signature 63 bytes") << inner("signedPreKey", "signature", b64(63));
    QTest::newRow("signedPreKey no signature") << innerWithout("signedPreKey", "signature");
    QTest::newRow("pq key 1568 bytes") << inner("pqSignedPreKey", "publicKey", b64(1568));
    QTest::newRow("negative id") << inner("signedPreKey", "id", -1);
    QTest::newRow("fractional id") << inner("signedPreKey", "id", 1.5);
    QTest::newRow("id above INT_MAX") << inner("signedPreKey", "id", 4294967295.0);
    QTest::newRow("id as text") << inner("signedPreKey", "id", "7");
    QTest::newRow("broken oneTimePreKey") << inner("oneTimePreKey", "publicKey", b64(10));
    QTest::newRow("oneTimePreKey not an object") << with("oneTimePreKey", 3);
    QTest::newRow("pqOneTimePreKey no signature") << innerWithout("pqOneTimePreKey", "signature");
}

void TestKeyBundleJson::rejectsBadReplies()
{
    QFETCH(QJsonObject, reply);
    QVERIFY(!fromKeysJson(reply));
}

// Real keys, real signatures, real homeserver: Bob uploads, Alice fetches, decodes and verifies.
// Needs a homeserver with the /user/keys routes (branch feat/server-key-storage), e.g.
//   Q3_KEYS_TEST_SERVER=http://127.0.0.1:8080 ./tst_keybundlejson
// Use a throwaway database: the test registers two new users.
void TestKeyBundleJson::endToEndThroughServer()
{
    const QString base = qEnvironmentVariable("Q3_KEYS_TEST_SERVER");
    if (base.isEmpty())
        QSKIP("Set Q3_KEYS_TEST_SERVER to a homeserver URL to run this test");

    QNetworkAccessManager net;
    const QString suffix = QString::number(QRandomGenerator::global()->bounded(1000000000));
    const QString bob = "bob" + suffix, alice = "alice" + suffix;
    QString tokens[2];
    for (int i = 0; i < 2; ++i) {
        const QJsonObject who{{"username", i ? alice : bob}, {"password", "pw" + suffix}};
        QCOMPARE(call(net, base, "POST", "/register", who).first, 201);
        const auto [status, body] = call(net, base, "POST", "/login", who);
        QCOMPARE(status, 200);
        tokens[i] = QJsonDocument::fromJson(body).object().value("authToken").toString();
        QVERIFY(!tokens[i].isEmpty());
    }

    // Bob's keys, signed with his identity, exactly as the app would build them.
    IdentityKeyPair identity;
    int curveId = 0, pqId = 100, curveBulkId = 10000, pqBulkId = 20000;
    auto spk = identity.signPublicPrekeyX25519(std::move(generatePreKeysX25519(1, curveId)[0]));
    auto pqspk = identity.signPublicPrekeyML_KEM_1024(std::move(generatePreKeysML_KEM_1024(1, pqId)[0]));
    auto curveBulk = identity.signBulkPublicPrekeysX25519(generatePreKeysX25519(3, curveBulkId));
    auto pqBulk = identity.signBulkPublicPrekeysML_KEM_1024(generatePreKeysML_KEM_1024(2, pqBulkId));
    const SharedKeyBytesStorage keys(identity.getBytes(), spk, pqspk, curveBulk, pqBulk);

    const auto [upStatus, upBody] = call(net, base, "POST", "/user/keys",
        QJsonObject{{"username", bob}, {"authToken", tokens[0]}, {"keys", toUploadJson(keys)}});
    QCOMPARE(upStatus, 200);
    QCOMPARE(QJsonDocument::fromJson(upBody).object(), (QJsonObject{{"oneTimePreKeys", 3}, {"pqOneTimePreKeys", 2}}));

    const auto [getStatus, getBody] = call(net, base, "GET", "/user/keys",
        QJsonObject{{"username", alice}, {"authToken", tokens[1]}, {"recipient", bob}});
    QCOMPARE(getStatus, 200);
    const auto bundle = fromKeysJson(QJsonDocument::fromJson(getBody).object().value("keys").toObject());
    QVERIFY(bundle);

    // Alice can prove every signed key came from Bob's identity, and gets his lowest one-time ids.
    IdentityKeyPublic bobPublic(bundle->identityKey);
    QVERIFY(bobPublic.checkSignature(bundle->signedPreKey));
    QVERIFY(bobPublic.checkSignature(bundle->pqSignedPreKey));
    QVERIFY(bundle->pqOneTimePreKey);
    QVERIFY(bobPublic.checkSignature(*bundle->pqOneTimePreKey));
    QVERIFY(bundle->oneTimePreKey);
    QCOMPARE(bundle->oneTimePreKey->identifier, 10000);
    QCOMPARE(bundle->pqOneTimePreKey->identifier, 20000);
    QCOMPARE(bundle->oneTimePreKey->key, curveBulk[0].key);

    // A key changed in transit must fail the check.
    auto tampered = bundle->signedPreKey;
    tampered.key[5] ^= 0x01;
    QVERIFY(!bobPublic.checkSignature(tampered));
}

QTEST_GUILESS_MAIN(TestKeyBundleJson)
#include "tst_keybundlejson.moc"
