cmake_minimum_required(VERSION 3.16)

find_package(Qt6 REQUIRED COMPONENTS Core)
find_package(OpenSSL 3.5 REQUIRED)

qt_add_library(Q3Crypto STATIC
    cryptography_primatives.h   cryptography_primatives.cpp
    SecretData.h
    clientkeystorage.h          clientkeystorage.cpp
    sharedKeyBytesStorage.h     sharedKeyBytesStorage.cpp
    identitykeypair.h           identitykeypair.cpp
    identitykeypublic.h         identitykeypublic.cpp
    otherclientpublickeys.h     otherclientpublickeys.cpp
    pqxdh_functions.h           pqxdh_functions.cpp
)
target_include_directories(Q3Crypto PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(Q3Crypto PUBLIC Qt6::Core OpenSSL::Crypto)
target_compile_features(Q3Crypto PUBLIC cxx_std_20)