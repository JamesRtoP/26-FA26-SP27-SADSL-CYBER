#include "pqxdh_key_manager.h"

std::vector<uint8_t> get_public_bytes(EVP_PKEY* pkey) {
    size_t len = 0;

    // 1. Try the raw method first (curve)
    if (EVP_PKEY_get_raw_public_key(pkey, NULL, &len) == 1) {
        std::vector<uint8_t> result(len);
        if (EVP_PKEY_get_raw_public_key(pkey, result.data(), &len) == 1) {
            return result;
        }
    }

    // 2. Fallback to universal DER encoding (required for ML-KEM-768 / post-quantum keys)
    int der_len = i2d_PUBKEY(pkey, NULL);
    if (der_len <= 0) {
        throw std::runtime_error("Failed to get public key DER length");
    }

    std::vector<uint8_t> result(der_len);
    unsigned char* ptr = result.data();
    if (i2d_PUBKEY(pkey, &ptr) <= 0) {
        throw std::runtime_error("Failed to encode public key to DER");
    }

    return result;
}

std::vector<uint8_t> PQXDH_Key_Manager::sign_data(const std::vector<uint8_t>& data) {
    EVP_MD_CTX *mctx = EVP_MD_CTX_new();
    EVP_PKEY_CTX *sctx = nullptr;

    if (EVP_DigestSignInit(mctx, &sctx, EVP_sha256(), NULL, this->identity_key_pair) <= 0) {
        EVP_MD_CTX_free(mctx);
        throw std::runtime_error("DigestSignInit failed");
    }

    size_t sig_len = 0;
    if (EVP_DigestSign(mctx, NULL, &sig_len, data.data(), data.size()) <= 0) {
        EVP_MD_CTX_free(mctx);
        throw std::runtime_error("Signature length query failed");
    }

    std::vector<uint8_t> signature(sig_len);
    if (EVP_DigestSign(mctx, signature.data(), &sig_len, data.data(), data.size()) <= 0) {
        EVP_MD_CTX_free(mctx);
        throw std::runtime_error("Signature generation failed");
    }

    EVP_MD_CTX_free(mctx);
    signature.resize(sig_len);
    return signature;
}

PQXDH_Key_Manager::PQXDH_Key_Manager()
{
    // 1. Generate Identity Key Pair (IK_B)
    EVP_PKEY_CTX *pctx = EVP_PKEY_CTX_new_id(EVP_PKEY_X25519, NULL);
    if (!pctx || EVP_PKEY_keygen_init(pctx) <= 0 || EVP_PKEY_keygen(pctx, &this->identity_key_pair) <= 0) {
        if (pctx) EVP_PKEY_CTX_free(pctx);
        throw std::runtime_error("Failed to generate Identity Key");
    }
    EVP_PKEY_CTX_free(pctx);

    // 2. Generate Signed Prekey Pair (SPK_B)
    pctx = EVP_PKEY_CTX_new_id(EVP_PKEY_X25519, NULL);
    if (!pctx || EVP_PKEY_keygen_init(pctx) <= 0 || EVP_PKEY_keygen(pctx, &(this->signed_prekey_pair)) <= 0) {
        if (pctx) EVP_PKEY_CTX_free(pctx);
        throw std::runtime_error("Failed to generate Signed Prekey");
    }
    EVP_PKEY_CTX_free(pctx);

    // 3. Generate Last-Resort PQ Signed Prekey (PQSPK_B) using ML-KEM-768
    this->pq_signed_prekey_pair = EVP_PKEY_Q_keygen(NULL, NULL, "ML-KEM-768");
    if (!this->pq_signed_prekey_pair) {
        throw std::runtime_error("Failed to generate ML-KEM-768 last-resort prekey");
    }
}

PQXDH_Key_Manager::~PQXDH_Key_Manager() {
    if (this->identity_key_pair) EVP_PKEY_free(this->identity_key_pair);
    if (this->signed_prekey_pair) EVP_PKEY_free(this->signed_prekey_pair);
    if (this->pq_signed_prekey_pair) EVP_PKEY_free(this->pq_signed_prekey_pair);
    for (auto& entry : this->curve_pool) EVP_PKEY_free(entry.key);
    for (auto& entry : this->pq_pool) EVP_PKEY_free(entry.key);
}


PublicKeyBundle PQXDH_Key_Manager::generate_base_bundle() {
    PublicKeyBundle bundle;
    bundle.curveIdentityKey = this->get_public_key_bytes(this->identity_key_pair);

    std::vector<uint8_t> spk_bytes = this->get_public_key_bytes(this->signed_prekey_pair);
    bundle.curvePreKey = {this->signed_prekey_pair_id, spk_bytes, this->sign_data(spk_bytes)};

    std::vector<uint8_t> pq_spk_bytes = this->get_public_key_bytes(this->pq_signed_prekey_pair);
    bundle.pq_public_key = {this->pq_signed_prekey_pair_id, pq_spk_bytes, this->sign_data(pq_spk_bytes)};

    return bundle;
}

std::vector<UnsignedPublicKey> PQXDH_Key_Manager::generate_curve_one_time_prekeys(int count) {
    std::vector<UnsignedPublicKey> list;
    for (int i = 0; i < count; ++i) {
        EVP_PKEY* opk = nullptr;
        EVP_PKEY_CTX *pctx = EVP_PKEY_CTX_new_id(EVP_PKEY_X25519, NULL);
        if (pctx && EVP_PKEY_keygen_init(pctx) > 0 && EVP_PKEY_keygen(pctx, &opk) > 0) {
            uint32_t id = this->curve_pool_id++;
            this->curve_pool.push_back({id, opk});
            list.push_back({id, this->get_public_key_bytes(opk)});
        }
        if (pctx) EVP_PKEY_CTX_free(pctx);
    }
    return list;
}

std::vector<SignedPublicKey> PQXDH_Key_Manager::generate_pq_one_time_prekeys(int count) {
    std::vector<SignedPublicKey> list;
    for (int i = 0; i < count; ++i) {
        EVP_PKEY* pq_opk = EVP_PKEY_Q_keygen(NULL, NULL, "ML-KEM-768");
        if (!pq_opk) continue;

        uint32_t id = this->pq_pool_id++;
        this->pq_pool.push_back({id, pq_opk});

        std::vector<uint8_t> pub_bytes = this->get_public_key_bytes(pq_opk);
        std::vector<uint8_t> sig = this->sign_data(pub_bytes);

        list.push_back({id, pub_bytes, sig});
    }
    return list;
}