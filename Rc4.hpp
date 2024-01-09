#include <random>
#include <vector>
#include <string>
#include <cstdint>


//! Не Rc4
class Rc4Handler
{
private:
    std::mt19937_64 gen;

public:
    Rc4Handler(std::int64_t root);
    void CryptDecrypt(std::string &data);
    void CryptDecrypt(std::vector<char>& data);
};
