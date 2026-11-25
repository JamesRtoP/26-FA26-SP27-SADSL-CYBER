#ifndef PQXDH_FUNCTIONS_H
#define PQXDH_FUNCTIONS_H

#include "cryptography_primatives.h"
#include "identitykeypublic.h"
#include "SecretData.h"

namespace Q3Crypto
{
    SecretData<160> concatinateKDFInputs(SecretData<32>& DH_1,SecretData<32>& DH_2,SecretData<32>& DH_3,SecretData<32>& DH_4,SecretData<32>& SS);
    std::vector<uint8_t> generateAD(const identityKeyPublicBytes& IK_A_bytes, const identityKeyPublicBytes& IK_B_bytes, const EVP_PKEY_ML_KEM_1024_ptr& PQPK_B);
}
#endif // PQXDH_FUNCTIONS_H
