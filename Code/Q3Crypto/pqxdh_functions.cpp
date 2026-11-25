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

}