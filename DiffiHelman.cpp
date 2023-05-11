#include "DiffiHelman.h"
#include <random>
#include <string>
#include <sstream>
#include <cstdint>
#include <optional>
auto GetGenerator(){
    static std::random_device rnd;
    static std::mt19937 generator{rnd()};
    return generator;
}

auto GetBase(){
    static auto pair = std::make_pair(bmp::int128_t(115), bmp::int128_t(2003000168));
    return pair;
}

bmp::int128_t GenRandomPreKey(std::int32_t mySecretKey){
    auto base = GetBase();
    return bmp::powm(base.first, bmp::int128_t(mySecretKey), base.second);
}

std::int32_t GenerateMySecretKey(){
    auto generator = GetGenerator();
    std::uniform_int_distribution<> distr(1000, 2000000000);
    return distr(generator);
}

std::optional<std::int32_t> GenFinalKey(std::int32_t anotherKey, std::int32_t mySecretKey){
    std::optional<std::int32_t> result;
    //std::int32_t anotherKeyI;
    bmp::int128_t anotherKeyMPZ = anotherKey;
    auto base = GetBase();
    bmp::int128_t mySecrMPZ = mySecretKey;
        //bmp::mpz_int anotherKeyMPZ = anotherKeyI;
    result= bmp::powm(anotherKeyMPZ, mySecrMPZ, base.second).convert_to<std::int32_t>();
    return result;
}
