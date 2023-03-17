#pragma once
#include <nlohmann/json.hpp>
#include <optional>
#include <exception>
using Json = nlohmann::json;
class NotImplementedException : public std::logic_error
{
public:
    NotImplementedException() : std::logic_error("Not implemented") {}
    virtual char const * what() const noexcept { return "Function not yet implemented."; }
};
template<class T>
Json ToJson(const T& data);
template <class T>
std::optional<T> FromJson(const Json& jsonData);
struct RegistrData{
    std::string login;
    std::string password;
};
struct AuthData{
    std::string login;
    std::string hash;
};
struct AuthDataAnswer{
    std::string passed;
};
struct KeyWord{
    std::string keyWord;
};
struct KeyWordRequest{
    std::string login;
};