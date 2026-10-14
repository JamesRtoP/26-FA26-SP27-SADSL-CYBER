#include "passwordhasher.h"

#include <openssl/rand.h>
#include <openssl/kdf.h>
#include <openssl/params.h>
#include <openssl/core_names.h>

#include <stdexcept>

std::vector<unsigned char> PasswordHasher::salt(size_t length)
{
    std::vector<unsigned char> salt(length);
    RAND_bytes(salt.data(), static_cast<int>(salt.size()));
    return salt;
}


std::vector<unsigned char> PasswordHasher::scryptHash(
    const QString& password,
    const std::vector<unsigned char> & salt,
    size_t outputKeyLength,
    uint64_t N,
    uint64_t r,
    uint64_t p
    ) {
    EVP_KDF* keyDerivationFunction = nullptr;
    EVP_KDF_CTX* keyDerivationFunctionContext = nullptr;

    ///kdf = Key Derivation function
    ///this is the function definition that kctx will use
    keyDerivationFunction = EVP_KDF_fetch(nullptr, "SCRYPT", nullptr);
    if (!keyDerivationFunction)
    {
        throw std::runtime_error("Failed to fetch scrypt");
    }

    ///kctx = key derivation function context
    ///This is the actual session that will hash the passowrd.
    keyDerivationFunctionContext = EVP_KDF_CTX_new(keyDerivationFunction);
    EVP_KDF_free(keyDerivationFunction);

    if(!keyDerivationFunctionContext)
    {
        throw std::runtime_error("Failed to create EVP_KDF_CTX");
    }

    OSSL_PARAM params[6];
    params[0] = OSSL_PARAM_construct_octet_string(OSSL_KDF_PARAM_PASSWORD,
                                                  (void*)password.data(), password.size());
    params[1] = OSSL_PARAM_construct_octet_string(OSSL_KDF_PARAM_SALT,
                                                  (void*)salt.data(), salt.size());
    params[2] = OSSL_PARAM_construct_uint64(OSSL_KDF_PARAM_SCRYPT_N, &N);
    params[3] = OSSL_PARAM_construct_uint64(OSSL_KDF_PARAM_SCRYPT_R, &r);
    params[4] = OSSL_PARAM_construct_uint64(OSSL_KDF_PARAM_SCRYPT_P, &p);
    params[5] = OSSL_PARAM_construct_end();

    std::vector<unsigned char> hash(outputKeyLength);
    int it_worked = EVP_KDF_derive(keyDerivationFunctionContext, hash.data(), hash.size(), params);

    EVP_KDF_CTX_free(keyDerivationFunctionContext);

    if (it_worked != 1) {
        throw std::runtime_error("Scrypt key derivation failed");
    }

    return hash;
}
