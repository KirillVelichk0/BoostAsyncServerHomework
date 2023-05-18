#include <random>
#include <vector>
#include <string>
#include <cstdint>

class Rc4Handler{
private:
    std::mt19937 gen;
public:
    Rc4Handler(std::int32_t root);
    void CryptDectypt(std::string& data);
};
