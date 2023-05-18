#pragma once
#include <boost/multiprecision/cpp_int.hpp>
#include <optional>
#include <cstdint>
namespace bmp = boost::multiprecision;
bmp::cpp_int GenRandomPreKey(std::int64_t mySecretKey, std::int64_t a, std::int64_t p);
std::int64_t GenerateMySecretKey();

std::optional<std::int64_t> GenFinalKey(std::int64_t anotherKey, std::int64_t mySecretKey, std::int64_t p);
