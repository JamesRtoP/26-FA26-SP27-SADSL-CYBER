#include "pqxdh_client.h"
#include <stdexcept>

static EVP_PKEY* bytes_to_public_key(int id_type, const std::vector<uint8_t>& bytes) {
    const unsigned char* ptr = bytes.data();
    return d2i_PUBKEY(NULL, &ptr, (long)bytes.size());
}

static EVP_PKEY* raw_bytes_to_x25519_public(const std::vector<uint8_t>& bytes) {
    return EVP_PKEY_new_raw_public_key(EVP_PKEY_X25519, NULL, bytes.data(), bytes.size());
}

bool PQXDH_Client::verify_signature(EVP_PKEY* ik_b, const std::vector<uint8_t>& data, const std::vector<uint8_t>& sig) {
    EVP_MD_CTX *mctx = EVP_MD_CTX_new();
    if (!mctx) return false;

    bool success = false;
    if (EVP_DigestVerifyInit(mctx, NULL, EVP_sha256(), NULL, ik_b) > 0) {
        if (EVP_DigestVerify(mctx, sig.data(), sig.size(), data.data(), data.size()) > 0) {
            success = true;
        }
    }
    EVP_MD_CTX_free(mctx);
    return success;
}

InitialMessage PQXDH_Client::send_initial_message(const PublicKeyBundle& bundle, const std::vector<uint8_t>& plaintext_message, std::vector<uint8_t>& out_shared_secret)
{
    EVP_PKEY* ik_b = raw_bytes_to_x25519_public(bundle.curveIdentityKey);
    EVP_PKEY* spk_b = raw_bytes_to_x25519_public(bundle.curvePreKey.public_key);

    //Verify signatures for Curve Identity Key
    if (!verify_signature(ik_b, bundle.curvePreKey.public_key, bundle.curvePreKey.signature)) {
        EVP_PKEY_free(ik_b);
        EVP_PKEY_free(spk_b);
        throw std::runtime_error("Invalid signature on Responder's Signed Prekey (SPK_B)");
    }

    //Choose either one-time key, or last resort key and verify signatures
    EVP_PKEY* pq_target_key = nullptr;
    uint32_t pq_prekey_id = 0;
    if (bundle.has_pq_one_time_prekey) {
        if (!verify_signature(ik_b, bundle.pq_one_time_prekey.public_key, bundle.pq_one_time_prekey.signature)) {
            EVP_PKEY_free(ik_b);
            EVP_PKEY_free(spk_b);
            throw std::runtime_error("Invalid signature on PQ One-Time Prekey");
        }
        pq_target_key = bytes_to_public_key(EVP_PKEY_NONE, bundle.pq_one_time_prekey.public_key);
        pq_prekey_id = bundle.pq_one_time_prekey.id;
    } else {
        if (!verify_signature(ik_b, bundle.pq_public_key.public_key, bundle.pq_public_key.signature)) {
            EVP_PKEY_free(ik_b);
            EVP_PKEY_free(spk_b);
            throw std::runtime_error("Invalid signature on Last-Resort PQ Prekey");
        }
        pq_target_key = bytes_to_public_key(EVP_PKEY_NONE, bundle.pq_public_key.public_key);
        pq_prekey_id = bundle.pq_public_key.id;
    }

    //generate ephemeral curve key pair
    EVP_PKEY* ek_a = nullptr;
    EVP_PKEY_CTX *pctx = EVP_PKEY_CTX_new_id(EVP_PKEY_X25519, NULL);
    if (!pctx || EVP_PKEY_keygen_init(pctx) <= 0 || EVP_PKEY_keygen(pctx, &ek_a) <= 0) {
        if (pctx) EVP_PKEY_CTX_free(pctx);
        EVP_PKEY_free(ik_b);
        EVP_PKEY_free(spk_b);
        if (pq_target_key) EVP_PKEY_free(pq_target_key);
        throw std::runtime_error("Failed to generate Ephemeral Key (EK_A)");
    }
    EVP_PKEY_CTX_free(pctx);


}

