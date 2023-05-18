#include "Rc4.hpp"
#include <iostream>
Rc4Handler::Rc4Handler(std::int64_t root) : gen(root) {}
void Rc4Handler::CryptDecrypt(std::string &data)
{
    for (char &byte : data)
    {
        auto random8byte=this->gen();
        std::cout << random8byte << std::endl;
        unsigned char random_byte = reinterpret_cast<unsigned char*>(&random8byte)[0];
        std::cout << int(random_byte) << std::endl;
        byte = byte ^ random_byte;
    }
}
void Rc4Handler::CryptDecrypt(std::vector<char> &data)
{
    for (char &byte : data)
    {
        auto random8byte=this->gen();
        unsigned char random_byte = reinterpret_cast<unsigned char*>(&random8byte)[0];
        std::cout << int(random_byte) << std::endl;
        byte = byte ^ random_byte;
    }
}
