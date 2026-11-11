#include "SecretData.h"

SecretData::SecretData(SecretData&& moveThingsOutOfMe) noexcept
{

}

SecretData& SecretData::operator=(SecretData&& moveThingsOutOfMe) noexcept
{

    return *this;
}

SecretData::SecretData()
{

}

SecretData::~SecretData()
{

}