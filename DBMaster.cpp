#include "DBMaster.h"
#include <iostream>
#include <openssl/sha.h>
#include <openssl/rand.h>
#include <fstream>
#include <base64_rfc4648.hpp>
auto GetMySqlDataFromJson()
{
    std::tuple<std::string, std::int32_t, std::string, std::string> result;
    using json = nlohmann::json;
    std::ifstream istr("../configs/mySqlDbConfig.json");
    std::cout << "startParsing" << std::endl;
    json sqlConfJson = json::parse(istr);
    std::cout << "first config parsed" << std::endl;
    istr.close();
    std::get<0>(result) = sqlConfJson["hostName"].get<std::string>();
    std::get<1>(result) = sqlConfJson["port"].get<std::int32_t>();
    std::get<2>(result) = sqlConfJson["user"].get<std::string>();
    std::string pathToPassword = sqlConfJson["pathToPasswordFile"].get<std::string>();
    istr.open(pathToPassword);
    json dbPassword = json::parse(istr);
    istr.close();
    std::get<3>(result) = dbPassword["password"].get<std::string>();
    return result;
}
auto BuildMySqlSessionFromJsonConfig()
{
    auto data = GetMySqlDataFromJson();
    auto [host, port, user, password] = data;
    mysqlx::Session result(host.c_str(), port, user.c_str(), password.c_str());
    return result;
}
DBMaster::DBMaster() : sess(BuildMySqlSessionFromJsonConfig()) {}
void DBMaster::SendRegistrDataToDB(const RegistrData &data)
{
    using namespace mysqlx;
    std::string hashed(32, 'a');
    std::cout << "startSending" << std::endl;
    std::string word(32, 'a');
    RAND_bytes(reinterpret_cast<unsigned char *>(word.data()), 32);
    SHA256(reinterpret_cast<const unsigned char *>(data.password.data()), data.password.size(), reinterpret_cast<unsigned char *>(hashed.data()));
    std::cout << "SHA is ok" << std::endl;
    Schema db = sess.getSchema("IBSystems");
    Table myColl = db.getTable("passcont");
    mysqlx::string login = data.login;
    mysqlx::bytes finalHash = hashed.c_str();
    mysqlx::bytes wordKey(word.c_str());
    std::cout << "Finally" << std::endl;
    try
    {
        std::cout << data.login.c_str() << " " << hashed.c_str() << std::endl;
        myColl.insert("uid", "login", "passH", "WordKey").values(0, login, finalHash, wordKey).execute();
    }
    catch (std::exception &err)
    {
        std::cout << "Uncorrect locale" << std::endl;
    }
}
std::string DBMaster::GetWordFromDb(const KeyWordRequest& keyRequest)
{
    std::string result;
    using namespace mysqlx;
    Schema db = sess.getSchema("IBSystems");
    Table myColl = db.getTable("passcont");
    mysqlx::string mLogin(keyRequest.login);
    auto opResult = myColl.select("WordKey").where("login = :login").bind("login", keyRequest.login).execute();
    if (opResult.count() != 0)
    {
        auto val = *opResult.begin();
        result = val[0].get<std::string>();
    }
    return result;
}
bool DBMaster::CompareSendedAuthDataWithDb(const AuthData &data)
{
    bool isOk = false;
    using namespace mysqlx;
    Schema db = sess.getSchema("IBSystems");
    Table myColl = db.getTable("passcont");
    mysqlx::string login(data.login);
    auto opResult = myColl.select("passH", "WordKey").where("login = :login").bind("login", login).execute();
    auto hashWithSalting = [](const char *salt, std::size_t salt_length, const char *input, std::size_t length, char *md)
    {
        SHA256_CTX context;
        if (!SHA256_Init(&context))
            return false;

        // first apply salt
        if (!SHA256_Update(&context, (unsigned char *)salt, salt_length))
            return false;

        // continue with data...
        if (!SHA256_Update(&context, (unsigned char *)input, length))
            return false;

        if (!SHA256_Final(reinterpret_cast<unsigned char*>(md), &context))
            return false;

        return true;
    };
    if (opResult.count() != 0)
    {
        auto val = *opResult.begin();
        std::string containedHash = val[0].get<std::string>();
        std::string containedWord = val[1].get<std::string>();
        std::string hash(32, 'a');
        bool isOpOk = hashWithSalting(containedWord.c_str(), containedWord.size(), containedHash.c_str(), containedHash.size(), hash.data());
        if(isOpOk){
            std::cout << hash << std::endl;
            std::cout << data.hash << std::endl;
            isOk = (hash == data.hash);
        }
    }
    return isOk;
}