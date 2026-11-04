#ifndef PQXDH_KEY_MANAGER_H
#define PQXDH_KEY_MANAGER_H

#include <vector>
#include <stdexcept>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/err.h>

struct SignedPublicKey {
    uint32_t id;
    std::vector<uint8_t> public_key; // PQOPK
    std::vector<uint8_t> signature;  // Sig(IK_B, EncodeKEM(PQOPK), Z)
};

struct UnsignedPublicKey
{
    uint32_t id;
    std::vector<uint8_t> public_key; // PQOPK
};

struct PublicKeyBundle {
    std::vector<uint8_t> curveIdentityKey;     // IK_B
    SignedPublicKey curvePreKey;           // SPK_B
    SignedPublicKey pq_public_key;           // PQPK_B (Last-resort or signed PQ prekey)

    // Optional one-time prekeys assigned for a specific handshake session
    bool has_one_time_prekey = false;
    UnsignedPublicKey curve_one_time_prekey;         // OPK_B
    bool has_pq_one_time_prekey = false;
    SignedPublicKey pq_one_time_prekey;      // PQOPK_B
};

class PQXDH_Key_Manager
{
private:
    ///EVP_PKEY holds a private and public key for any algorithm
    EVP_PKEY* identity_key_pair = nullptr;// IK_B
    EVP_PKEY* signed_prekey_pair = nullptr;// SPK_B
    EVP_PKEY* pq_signed_prekey_pair = nullptr; // PQSPK_B

    uint32_t signed_prekey_pair_id = 10;

    uint32_t pq_signed_prekey_pair_id = 20;

    uint32_t curve_pool_id = 1000;

    uint32_t pq_pool_id = 2000;

    struct InternalKey
    {
        uint32_t id;
        EVP_PKEY* key;
    };

    std::vector<InternalKey> curve_pool;
    std::vector<InternalKey> pq_pool;

    std::vector<uint8_t> get_public_key_bytes(EVP_PKEY* pkey);
    std::vector<uint8_t> sign_data(const std::vector<uint8_t>& data);
public:
    PQXDH_Key_Manager();
    ~PQXDH_Key_Manager();
    std::vector<UnsignedPublicKey> generate_curve_one_time_prekeys(int count);
    std::vector<SignedPublicKey> generate_pq_one_time_prekeys(int count);
    PublicKeyBundle generate_base_bundle();
};

#endif // PQXDH_KEY_MANAGER_H
