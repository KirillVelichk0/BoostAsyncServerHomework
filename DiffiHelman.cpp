#include "DiffiHelman.h"
#include <random>
#include <string>
#include <sstream>
#include <cstdint>
#include <optional>
auto& GetGenerator(){
    static std::random_device rnd;
    static std::mt19937 generator{rnd()};
    return generator;
}

auto GetBase(){
    static auto pair = std::make_pair(bmp::cpp_int(115), bmp::cpp_int(2003000168));
    return pair;
}

bmp::cpp_int GenRandomPreKey(std::int64_t mySecretKey, std::int64_t a, std::int64_t p){
    return bmp::powm(bmp::cpp_int(a), bmp::cpp_int(mySecretKey), bmp::cpp_int(p));
}

std::int64_t GenerateMySecretKey(){
    auto& generator = GetGenerator();
    std::uniform_int_distribution<> distr(1000, 2000000000);
    return distr(generator);
}

std::optional<std::int64_t> GenFinalKey(std::int64_t anotherKey, std::int64_t mySecretKey, std::int64_t p){
    std::optional<std::int64_t> result;
    //std::int32_t anotherKeyI;
    bmp::cpp_int anotherKeyMPZ = anotherKey;
    auto base = GetBase();
    bmp::cpp_int mySecrMPZ = mySecretKey;
        //bmp::mpz_int anotherKeyMPZ = anotherKeyI;
    result= bmp::powm(anotherKeyMPZ, mySecrMPZ, bmp::cpp_int(p)).convert_to<std::int64_t>();
    return result;
}
