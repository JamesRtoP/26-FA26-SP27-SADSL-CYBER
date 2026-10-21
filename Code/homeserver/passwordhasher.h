#ifndef PASSWORDHASHER_H
#define PASSWORDHASHER_H

#include <vector>
#include <qstring.h>

class PasswordHasher
{
public:

    static std::vector<unsigned char> salt(size_t length = 16);
    static std::vector<unsigned char> scryptHash(
        const QString& password,
        const std::vector<unsigned char> & salt,
        size_t outputKeyLength = 32,
        uint64_t N = 16384,
        uint64_t r = 8,
        uint64_t p =1
        );

};

#endif // PASSWORDHASHER_H
