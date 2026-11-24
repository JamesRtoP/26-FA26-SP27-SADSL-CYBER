#ifndef PQXDH_CLIENT_H
#define PQXDH_CLIENT_H

#include <vector>
#include <string>
#include <openssl/evp.h>
#include <openssl/err.h>
#include <openssl/kdf.h>
#include "pqxdh_key_manager.h"


struct InitialMessage {
    std::vector<uint8_t> identity_key_a;     // IK_A (Sender's Identity Key)
    std::vector<uint8_t> ephemeral_key_a;    // EK_A (Sender's Ephemeral Key)
    uint32_t used_spk_id;                   // SPK_B ID used
    bool has_opk = false;
    uint32_t used_opk_id;                   // OPK_B ID used (if any)
    uint32_t used_pq_prekey_id;             // PQPK_B / PQOPK_B ID used
    std::vector<uint8_t> pq_ciphertext;      // ML-KEM Ciphertext (CT)
    std::vector<uint8_t> ciphertext;         // Encrypted payload message
};

class PQXDH_Client
{
private:
    bool verify_signature(EVP_PKEY* ik_b, const std::vector<uint8_t>& data, const std::vector<uint8_t>& sig);
    std::vector<uint8_t> perform_dh(EVP_PKEY* priv_key, EVP_PKEY* pub_key);
public:
    PQXDH_Client();
    InitialMessage send_initial_message(const PublicKeyBundle& bundle, const std::vector<uint8_t>& plaintext_message, std::vector<uint8_t>& out_shared_secret);
};

#endif // PQXDH_CLIENT_H
