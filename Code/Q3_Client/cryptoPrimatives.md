- [X] curve25519 Wrapper
- [X] X25519 Wrapper (Elliptic-Curve Diffie-Hellman (ECDH) key agreement protocol) DH(PK1, PK2)
- [X] Ed25519 Wrapper (Edwards-curve Digital Signature Algorithm (EdDSA)) (Replacing XEdDSA) Sig(PK, M, Z)
- [X] SHA 512 Wrapper
- [X] info string of at least 8 bytes
- [X] pqkem (ML-KEM)
- [ ] AEAD (The PQXDH white paper says AES256 is sufficient)
- [X] HKDF

Openssl is being used for [PQXDH](https://signal.org/docs/specifications/pqxdh/pqxdh.pdf "PQXDH Specification") primatives. Openssl does not have an implmentation of [XEdDSA](https://signal.org/docs/specifications/xeddsa/ "XEdDSA Specification"). XEdDSA allows a single key pair to be used for either or both elliptic curve Diffie-Hellman and Signing. Instead of implementing this ourselves, we have decided to 

The PQXDH document from Signal refers to pqkem as "A post-quantum key encapsulation mechanism that has IND-CCA post-quantum security (e.g. Crystals-Kyber-1024)". Crystals-Kyber-1024 is the fullname of ML-KEM as standardized by [FIPS 203](https://www.nist.gov/news-events/news/2024/08/nist-releases-first-3-finalized-post-quantum-encryption-standards "NIST Regulation"). 


## Algorithms

### X25519
X25519 is an Elliptic Curve Diffie-Hellman (ECDH) key agreement algorithm utilizing curve 25519. It generates a shared key by combining a someone's public key with someone else's private key. This key is the same if you sitch who's private key is being used.

## Data Types

### Secret Values
Values that are intended to remain secret need their own data type. Utilizing arrays or vectors can lead to multiple copies of a secret in memory and no guarentee that the memory is overwritten after the memory is dealocated.
This new data type should only allow moves.

### Fixed-Length Values

## Navigating Openssl

### Termonology

#### EVP
EVP stands for EnVeloPe. It is how Openssl stores keys, message digests, and more in a generic type. For instance we will be storing both X25519 keys and ML-KEM keys in EVP_PKEY.

#### MD
MD stands for message digest. This is aparently a common term for the fixed-length hash produced by a hashing function.

#### CTX
CTX stands for context. From my understanding this context is where the actual crytography work takes place.

### Using An Algorithm


#### 1. Fetch the algorithm
- Use EVP\_*type*\_fetch() to fetch the algorithm
- Set an EVP\_*type* pointer to the fetched algorithm
- Check to ensure the EVP\_*type* pointer is not null

``` cpp
EVP_MD* messageDigest = EVP_MD_fetch(nullptr, "SHA512", nullptr);
if (!messageDigest){throw std::runtime_error("Failed to fetch EVP_MD");}
```

#### 2. Create the context
- Use EVP\_*type*\_CTX\_new() to create a context
- Set an EVP\_*type*\_CTX pointer to the created context
- Check to ensure the EVP\_*type*\_CTX pointer is not null

``` cpp
EVP_MD_CTX* context = EVP_MD_CTX_new();
    if(!context)
    {
        EVP_MD_free(messageDigest);
        throw std::runtime_error("Failed to create EVP_MD_CTX");
    }
```

Step 1 and 2 are interchangable, but make sure that the second free memory allocated the in the first if it fails.

#### 3. Add algorithm to context

``` cpp
    bool success = EVP_DigestInit_ex2(context, messageDigest, nullptr) == 1;
    if(!success)
    {
        EVP_MD_CTX_free(context);
        EVP_MD_free(messageDigest);
        throw std::runtime_error("Failed to Add Algorithm to Context");
    }
```

#### 4. Update context with input

``` cpp
    // std::vector<uint8_t> plaintext is predefined
    success = EVP_DigestUpdate(context, plaintext.data(), plaintext.size()) == 1;
    if(!success)
    {
        EVP_MD_CTX_free(context);
        EVP_MD_free(messageDigest);
        throw std::runtime_error("Failed to Add Message to Context");
    }
```

This step can be repeated with different updates or to add more messages.

#### 5. Create output variables
- Variable for output data
- Variable for output size
``` cpp
    std::vector<uint8_t> hash(EVP_MAX_MD_SIZE);
    unsigned int outputSize = 0;
```

#### 6. Execute context
``` cpp
    success = EVP_DigestFinal_ex(context, hash.data(), &outputSize) == 1;
    if(!success)
    {
        EVP_MD_CTX_free(context);
        EVP_MD_free(messageDigest);
        throw std::runtime_error("Failed to Execute Context");
    }
```

#### 7. Free memory
``` cpp
    EVP_MD_CTX_free(context);
    EVP_MD_free(messageDigest);
```

- [ ] Bob’s curve identity key IK<sub>B</sub> (This was originally one key using XEdDSA, but we will have to split it into 2 keys: one for X25519 and one for Ed25519)
- [ ] Bob’s signed curve prekey and its identifier (SPK<sub>B</sub>, IdEC(SPK<sub>B</sub>)) (This is just an X25519 key with an int id)
- [ ] Bob’s signature on the curve prekey Sig(IK<sub>B</sub>, EncodeEC(SPK<sub>B</sub>), Z<sub>SPK</sub>) (signed using IK<sub>B</sub>. In our case this is the Ed25519 key)
- [ ]
- [ ]
- [ ]
- [ ]
- [ ]