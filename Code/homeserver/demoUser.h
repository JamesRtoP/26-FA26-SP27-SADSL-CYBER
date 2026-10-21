#ifndef DEMOUSER_H
#define DEMOUSER_H

#include "qstring.h"

struct DemoUser{
    QString username;
    std::vector<unsigned char> passwordHash;
    std::vector<unsigned char> salt;
};

#endif // DEMOUSER_H
