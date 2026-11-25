#include "pqxdh_functions.h"

namespace Q3Crypto
{

    SecretData<160> concatinateKDFInputs(SecretData<32>& DH_1,SecretData<32>& DH_2,SecretData<32>& DH_3,SecretData<32>& DH_4,SecretData<32>& SS)
    {
        SecretData<160> KDFInput;
        std::copy(DH_1.data(), DH_1.data()+DH_1.size(), KDFInput.data());
        DH_1.wipe();
        std::copy(DH_2.data(), DH_2.data()+DH_2.size(), KDFInput.data()+32);
        DH_2.wipe();
        std::copy(DH_3.data(), DH_3.data()+DH_3.size(), KDFInput.data()+64);
        DH_3.wipe();
        std::copy(DH_4.data(), DH_4.data()+DH_4.size(), KDFInput.data()+96);
        DH_4.wipe();
        std::copy(SS.data(), SS.data()+DH_4.size(), KDFInput.data()+128);
        SS.wipe();
        return KDFInput;
    }

    std::vector<uint8_t> generateAD(const identityKeyPublicBytes& IK_A_bytes, const identityKeyPublicBytes& IK_B_bytes, const EVP_PKEY_ML_KEM_1024_ptr& PQPK_B)
    {
        std::array<uint8_t, PublicKeySizeML_KEM_1024 + 1> PQPK_B_bytes = encodePublicML_KEM_1024(PQPK_B);
        std::vector<uint8_t> AD;
        AD.reserve(IK_A_bytes.Ed25519_key.size() + IK_A_bytes.X25519_key.size() + IK_B_bytes.Ed25519_key.size() + IK_B_bytes.X25519_key.size() + PQPK_B_bytes.size());
        int offset = 0;

        AD.insert(AD.end(), IK_A_bytes.Ed25519_key.begin(), IK_A_bytes.Ed25519_key.begin());
        AD.insert(AD.end(), IK_A_bytes.X25519_key.begin(), IK_A_bytes.X25519_key.begin());
        AD.insert(AD.end(), IK_B_bytes.Ed25519_key.begin(), IK_B_bytes.Ed25519_key.begin());
        AD.insert(AD.end(), IK_A_bytes.X25519_key.begin(), IK_A_bytes.X25519_key.begin());
        AD.insert(AD.end(), PQPK_B_bytes.begin(), PQPK_B_bytes.begin());
        return AD;
    }

    QJsonObject initialMessageToJson(const PQXDHInitialMessage& message)
    {
        QJsonObject json;
        json["identityKey"] = bytesToBase64(message.IK_B.X25519_key.data(), message.IK_B.X25519_key.size());
        json["identitySigningKey"] = bytesToBase64(message.IK_B.Ed25519_key.data(), message.IK_B.Ed25519_key.size());
        json["ephemeralKey"] = bytesToBase64(message.EK_B.data(), message.EK_B.size());
        json["pqCiphertext"] = bytesToBase64(message.PQKEMCiphertext.data(), message.PQKEMCiphertext.size());

        json["signedPreKeyId"] = message.keyIdentifiers.SPK_B_Identifier;
        json["pqPreKeyId"] = message.keyIdentifiers.PQPK_B_Identifier;
        if (message.keyIdentifiers.OPK_B_Identifier >= 0)          // -1 means no one-time curve prekey was used
            json["oneTimePreKeyId"] = message.keyIdentifiers.OPK_B_Identifier;

        json["nonce"] = bytesToBase64(message.initialCiphertext.nonce.data(), message.initialCiphertext.nonce.size());
        json["ciphertext"] = bytesToBase64(message.initialCiphertext.ciphertext.data(), message.initialCiphertext.ciphertext.size());
        json["tag"] = bytesToBase64(message.initialCiphertext.tag.data(), message.initialCiphertext.tag.size());
        return json;
    }

    PQXDHInitialMessage initialMessageFromJson(const QJsonObject& json)
    {
        PQXDHInitialMessage message;

        // ---- fixed-size fields ----
        base64FieldToBytes(json, "identityKey",
                           message.IK_B.X25519_key.data(), message.IK_B.X25519_key.size());
        base64FieldToBytes(json, "identitySigningKey",
                           message.IK_B.Ed25519_key.data(), message.IK_B.Ed25519_key.size());
        base64FieldToBytes(json, "ephemeralKey", message.EK_B.data(), message.EK_B.size());
        base64FieldToBytes(json, "pqCiphertext", message.PQKEMCiphertext.data(), message.PQKEMCiphertext.size());
        base64FieldToBytes(json, "nonce",
                           message.initialCiphertext.nonce.data(), message.initialCiphertext.nonce.size());
        base64FieldToBytes(json, "tag",
                           message.initialCiphertext.tag.data(), message.initialCiphertext.tag.size());

        // ---- key identifiers ----
        QJsonValue signedPreKeyId = json.value("signedPreKeyId");
        if (!signedPreKeyId.isDouble() || signedPreKeyId.toInt() < 0
            || static_cast<double>(signedPreKeyId.toInt()) != signedPreKeyId.toDouble())
            throw std::runtime_error("JSON: 'signedPreKeyId' is missing or not a valid ID");
        message.keyIdentifiers.SPK_B_Identifier = signedPreKeyId.toInt();

        QJsonValue pqPreKeyId = json.value("pqPreKeyId");
        if (!pqPreKeyId.isDouble() || pqPreKeyId.toInt() < 0
            || static_cast<double>(pqPreKeyId.toInt()) != pqPreKeyId.toDouble())
            throw std::runtime_error("JSON: 'pqPreKeyId' is missing or not a valid ID");
        message.keyIdentifiers.PQPK_B_Identifier = pqPreKeyId.toInt();

        if (json.contains("oneTimePreKeyId"))
        {
            QJsonValue oneTimePreKeyId = json.value("oneTimePreKeyId");
            if (!oneTimePreKeyId.isDouble() || oneTimePreKeyId.toInt() < 0
                || static_cast<double>(oneTimePreKeyId.toInt()) != oneTimePreKeyId.toDouble())
                throw std::runtime_error("JSON: 'oneTimePreKeyId' is not a valid ID");
            message.keyIdentifiers.OPK_B_Identifier = oneTimePreKeyId.toInt();
        }
        else
        {
            message.keyIdentifiers.OPK_B_Identifier = -1;           // Alice used no one-time curve prekey
        }

        // ---- variable-length ciphertext ----
        QJsonValue ciphertextValue = json.value("ciphertext");
        if (!ciphertextValue.isString())
            throw std::runtime_error("JSON: 'ciphertext' is missing or not a string");
        QByteArray::FromBase64Result decoded = QByteArray::fromBase64Encoding(
            ciphertextValue.toString().toLatin1(), QByteArray::AbortOnBase64DecodingErrors);
        if (!decoded)
            throw std::runtime_error("JSON: 'ciphertext' is not valid Base64");
        message.initialCiphertext.ciphertext.assign(decoded.decoded.begin(), decoded.decoded.end());

        return message;
    }

}