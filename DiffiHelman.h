#pragma once
#include <boost/multiprecision/cpp_int.hpp>
#include <optional>
#include <cstdint>
namespace bmp = boost::multiprecision;
bmp::int128_t GenRandomPreKey(std::int32_t mySecretKey);

std::int32_t GenerateMySecretKey();

std::optional<std::int32_t> GenFinalKey(std::int32_t anotherKey, std::int32_t mySecretKey);
