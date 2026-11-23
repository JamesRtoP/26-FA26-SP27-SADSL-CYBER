#ifndef KEYBUNDLEJSON_H
#define KEYBUNDLEJSON_H

// Converts public key bundles between the Q3Crypto byte structs and the JSON used by the
// homeserver's /user/keys routes. Every key and signature travels as standard base64, and
// each key keeps its leading type byte so the decodePublic* functions can read it.
// The JSON format is documented in Code/homeserver/DATABASE.md, section "Public key API".

#include "cryptography_primatives.h"
#include "sharedKeyBytesStorage.h"

#include <QJsonObject>
#include <optional>

namespace Q3Crypto
{
    // One handshake's worth of another user's public keys, as returned by GET /user/keys.
    struct ReceivedKeyBundle
    {
        identityKeyPairBytes identityKey;                          // X25519 + Ed25519 halves
        prekeyWithIdentifierSignedBytesX25519 signedPreKey;        // SPK
        prekeyWithIdentifierSignedBytesML_KEM_1024 pqSignedPreKey; // last-resort PQ prekey
        std::optional<prekeyWithIdentifierBytesX25519> oneTimePreKey;               // absent when the pool was empty
        std::optional<prekeyWithIdentifierSignedBytesML_KEM_1024> pqOneTimePreKey;  // absent when the pool was empty
    };

    /**
     * @brief Build the "keys" object for POST /user/keys (ClientService::pushPublicKeys).
     * @param keys Your public, signed keys.
     * @return {identityKey, identitySigningKey, signedPreKey, pqSignedPreKey, oneTimePreKeys, pqOneTimePreKeys}.
     * @note Curve one-time prekeys are sent without their signature; the server does not store it.
     */
    QJsonObject toUploadJson(const SharedKeyBytesStorage &keys);

    /**
     * @brief Read the "keys" object returned by GET /user/keys (ClientService::getPublicKeys).
     * @param keys The object under "keys" in the server's reply.
     * @return The decoded bundle, or std::nullopt if a required field is missing, any value is not
     *         strict base64, any key or signature has the wrong length, or an id is out of range.
     * @note Does not check signatures. Verify them with IdentityKeyPublic before using the keys.
     */
    std::optional<ReceivedKeyBundle> fromKeysJson(const QJsonObject &keys);
}

#endif // KEYBUNDLEJSON_H
