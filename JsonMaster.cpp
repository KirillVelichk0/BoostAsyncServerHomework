#include "JsonMaster.h"
#include <base64_rfc4648.hpp>
#include <iostream>
using Base64Url = cppcodec::base64_rfc4648;
auto FromVecToString(const std::vector<uint8_t> &data)
{
    return std::string(data.cbegin(), data.cend());
}
template <>
Json ToJson<RegistrData>(const RegistrData &data)
{
    throw NotImplementedException();
}
template <>
Json ToJson<AuthData>(const AuthData &data)
{
    throw NotImplementedException();
}
template <>
Json ToJson<KeyWord>(const KeyWord &data)
{
    Json result;
    try
    {
        result["KeyWord"] = Base64Url::encode(data.keyWord);
        result["Type"] = "KeyWordAnswer";
        return result;
    }
    catch (std::exception &ex)
    {
        throw ex;
    }
}
template <>
Json ToJson<AuthDataAnswer>(const AuthDataAnswer &answer)
{
    Json result;
    try
    {
        result["Answer"] = answer.passed;
        result["Type"] = "AuthAnswer";
        return result;
    }
    catch (std::exception &ex)
    {
        throw ex;
    }
}
template <>
std::optional<KeyWordRequest> FromJson(const Json &jsonData)
{
    std::optional<KeyWordRequest> result;
    try
    {
        auto login = jsonData.find("username");
        std::cout << "finded" << std::endl;
        if (login != jsonData.cend())
        {
            std::cout << "Json linking" << std::endl;
            result = KeyWordRequest{login : FromVecToString(Base64Url::decode(login->get<std::string>()))};
            std::cout << "Json linked" << std::endl;
            std::cout << result.value().login << std::endl;
        }
        return result;
    }
    catch (std::exception &ex)
    {
        result.reset();
        return result;
    }
}
template <>
std::optional<AuthData> FromJson(const Json &jsonData)
{
    std::optional<AuthData> result;
    try
    {
        auto login = jsonData.find("username");
        auto hash = jsonData.find("hash");
        std::cout << "finded" << std::endl;
        if (login != jsonData.cend() && hash != jsonData.cend())
        {
            std::cout << "Json linking" << std::endl;
            result = AuthData{login : FromVecToString(Base64Url::decode(login->get<std::string>())),
                              hash : FromVecToString(Base64Url::decode(hash->get<std::string>()))};
            std::cout << "Json linked" << std::endl;
            std::cout << result.value().login << " " << result.value().hash << std::endl;
        }
        return result;
    }
    catch (std::exception &ex)
    {
        result.reset();
        return result;
    }
}
template <>
std::optional<RegistrData> FromJson(const Json &jsonData)
{
    std::optional<RegistrData> result;
    try
    {
        auto login = jsonData.find("username");
        auto password = jsonData.find("password");
        std::cout << "finded" << std::endl;
        if (login != jsonData.cend() && password != jsonData.cend())
        {
            std::cout << "Json linking" << std::endl;
            result = RegistrData{login : FromVecToString(Base64Url::decode(login->get<std::string>())),
                                 password : FromVecToString(Base64Url::decode(password->get<std::string>()))};
            std::cout << "Json linked" << std::endl;
            std::cout << result.value().login << " " << result.value().password << std::endl;
        }
        return result;
    }
    catch (std::exception &ex)
    {
        result.reset();
        return result;
    }
}