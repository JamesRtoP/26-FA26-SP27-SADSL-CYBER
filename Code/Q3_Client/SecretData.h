#ifndef SECRETDATA_H
#define SECRETDATA_H

#include <array>
#include <span>

template <std::size_t SIZE>
class SecretData
{
    static_assert(SIZE>0, "SecretData size must be positive");
private:
    std::array<uint8_t, SIZE> secret;
public:
    static constexpr std::size_t kSize = SIZE;
    // Remove Copy Constructor and Copy Assignment Operator
    SecretData(const SecretData& copyMe) = delete;
    SecretData& operator=(const SecretData& copyMe) = delete;

    SecretData(SecretData&& moveThingsOutOfMe) noexcept
    {
        this->secret = moveThingsOutOfMe.secret;
        moveThingsOutOfMe.wipe();
    }
    SecretData& operator=(SecretData&& moveThingsOutOfMe) noexcept
    {
        if (this != &moveThingsOutOfMe)
        {
            this->secret = moveThingsOutOfMe.secret;
            moveThingsOutOfMe.wipe();
        }
        return *this;
    }

    SecretData() noexcept {this->secret.fill(0);};
    ~SecretData() {this->wipe();};

    void wipe() noexcept { OPENSSL_cleanse(this->secret.data(), SIZE); }
    static constexpr std::size_t size() noexcept { return SIZE;}

    bool equals(const SecretData& compareMe) const noexcept
    {
        return CRYPTO_memcmp(this->secret.data(), compareMe.secret.data(), SIZE) == 0;
    }

    std::uint8_t*       data() noexcept { return this->secret.data(); }
    const std::uint8_t* data() const noexcept { return this->secret.data(); }

    std::span<std::uint8_t, SIZE>       span() noexcept { return this->secret; }
    std::span<const std::uint8_t, SIZE> span() const noexcept { return this->secret; }
};

#endif // SECRETDATA_H
