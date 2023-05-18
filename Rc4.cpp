#include "Rc4.hpp"
#include <climits>
Rc4Handler::Rc4Handler(std::int32_t root): gen(root){}
void Rc4Handler::CryptDectypt(std::string& data){
    std::uniform_real_distribution<> unif(0, 255);
    for(char& byte: data){
        char random_byte = unif(this->gen) + CHAR_MIN;
        byte = byte ^ random_byte;
    }
}
