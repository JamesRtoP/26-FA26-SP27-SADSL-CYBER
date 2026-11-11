#include <QtTest>
#include <vector>
#include <array>
#include <string>
#include "cryptography_primatives.h"

class TestCryptoPrimatives : public QObject
{
    Q_OBJECT

private slots:
    void cleanup();

    void sha512_data();
    void sha512();
};

void TestCryptoPrimatives::cleanup()
{

}

void TestCryptoPrimatives::sha512_data()
{
    QTest::addColumn<std::vector<uint8_t>>("plaintext");
    QTest::addColumn<std::array<uint8_t, 64>>("hash");
    std::vector<std::string> inputs =
        {"wassup"};
    std::vector<std::vector<uint8_t>> hashes =
        {Q3Crypto::hexToBytes("b135eac4b195891c79f7d6a7cac21004d8dd28c4349109a27302bda272190ab1f0f6eba49d3f1a9e9c666eccde3df9aafd5463c412b7113efe9d0c707b23d057")};
    for(int i = 0; i < inputs.size(); i ++)
    {
        std::array<uint8_t, 64> array;
        std::copy_n(hashes[i].begin(), 64, array.begin());
        QTest::newRow(inputs[i].data()) << std::vector<uint8_t>(inputs[i].begin(), inputs[i].end()) << array;
    }
    std::array<uint8_t, 64> arr;
    std::vector<uint8_t> hash = Q3Crypto::hexToBytes("cf83e1357eefb8bdf1542850d66d8007d620e4050b5715dc83f4a921d36ce9ce47d0d13c5d85f2b0ff8318d2877eec2f63b931bd47417a81a538327af927da3e");
    std::copy_n(hash.begin(), 64, arr.begin());
    QTest::newRow("Empty") << std::vector<uint8_t>() << arr;
}

// Necessary for QFetch sintax
// QFETCH actually sees the comma in <uint8_t, 64> and thinks there is an extra argument
using HashArray = std::array<uint8_t, 64>;

void TestCryptoPrimatives::sha512()
{
    QFETCH(std::vector<uint8_t>, plaintext);
    QFETCH(HashArray, hash);
    QCOMPARE(Q3Crypto::sha512(plaintext), hash);
}

QTEST_GUILESS_MAIN(TestCryptoPrimatives)
#include "testCryptoPrimatives.moc"