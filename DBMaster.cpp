#include "DBMaster.h"
#include <iostream>
#include <openssl/sha.h>
#include <openssl/rand.h>
#include <fstream>
#include <string>
#include <base64_rfc4648.hpp>
using namespace std::string_literals;
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
auto BuildMySqlSessionFromJsonConfig(boost::asio::io_context& ctx, boost::asio::ssl::context& ssl_ctx)
{
    auto data = GetMySqlDataFromJson();
    auto [host, port, user, password] = data;
    boost::asio::ip::tcp::resolver resolver(ctx.get_executor());
    auto endpoint = resolver.resolve(host, boost::mysql::default_port_string);
    boost::mysql::handshake_params params(user, password, "IBSystems");
    boost::mysql::tcp_ssl_connection conn(ctx.get_executor(), ssl_ctx);
    boost::mysql::error_code code;
    boost::mysql::diagnostics diag;
    
    conn.connect(*endpoint.begin(), params, code, diag);
    std::cout << "Connected to db with code " << code << std::endl;
    std::cout << "Diag: " << diag.server_message() << std::endl;
    return conn;
}
DBMaster::DBMaster(boost::asio::io_context& ctx, boost::asio::ssl::context& ssl_ctx) : conn(BuildMySqlSessionFromJsonConfig(ctx, ssl_ctx)) {}
void DBMaster::SendRegistrDataToDB(const RegistrData &data)
{
    std::string hashed(32, 'a');
    std::cout << "startSending" << std::endl;
    std::string word(32, 'a');
    RAND_bytes(reinterpret_cast<unsigned char *>(word.data()), 32);
    SHA256(reinterpret_cast<const unsigned char *>(data.password.data()), data.password.size(), reinterpret_cast<unsigned char *>(hashed.data()));
    std::cout << "SHA is ok" << std::endl;
    boost::mysql::results que_result;
    char statement[] = "insert into passcont(uid, login, passH, WordKey) VALUES(?, ?, ?, ?)";
      std::cout << statement << std::endl;
    boost::mysql::error_code er; boost::mysql::diagnostics diag;
    boost::mysql::statement st = conn.prepare_statement(statement, er, diag);
    std::cout << er.value() << " " << er.message() << " " << diag.server_message() << std::endl;
    if (er.value()){
        return;
    }
    std::cout << "Finally" << std::endl;
    try
    {
        auto GetBlobFromString = [](std::string& data){
            std::vector<unsigned char> init_vec;
            std::copy(data.begin(), data.end(), std::back_inserter(init_vec));
            return boost::mysql::blob(init_vec);
        };
        boost::mysql::blob hashedBlob = GetBlobFromString(hashed);
        boost::mysql::blob wordKeyBlob = GetBlobFromString(word);
        std::cout << data.login.c_str() << " " << hashed.c_str() << std::endl;
        this->conn.execute_statement(st, std::make_tuple(0, data.login, hashedBlob, wordKeyBlob), que_result);
        
    }
    catch (std::exception &err)
    {
        std::cout << "Uncorrect locale" << std::endl;
    }
}
std::string DBMaster::GetWordFromDb(const KeyWordRequest& keyRequest)
{
    std::string result;
    boost::mysql::statement st = this->conn.prepare_statement("SELECT WordKey from passcont WHERE \
    login = ?");
    boost::mysql::results que_res;
    this->conn.execute_statement(st, std::make_tuple(keyRequest.login), que_res);
    auto opResult = que_res.rows();
    if (opResult.size() != 0)
    {
        auto val = opResult.at(0);;
        auto containedWordB = val.at(0).as_blob();
        std:: cout << "Word getted" << std::endl;
        std::copy(containedWordB.begin(), containedWordB.end(), std::back_inserter(result));
        std::cout << "KeyWord to string" << std::endl;
    }
    return result;
}
bool DBMaster::CompareSendedAuthDataWithDb(const AuthData &data)
{
    bool isOk = false;
    boost::mysql::statement st = this->conn.prepare_statement("SELECT passH, WordKey from passcont WHERE\
    login = ?");
    boost::mysql::results que_res;
    this->conn.execute_statement(st, std::make_tuple(data.login), que_res);
    auto opResult = que_res.rows();
    std::cout << "Rows getted" << std::endl;
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
    if (opResult.size() != 0)
    {
        auto val = opResult.at(0);
        std::cout << "containers getting" << std::endl;
        auto containedHashB = val.at(0).as_blob();
        std::cout << "Hash getted" << std::endl;
        std::string containedHash;
        std::copy(containedHashB.begin(), containedHashB.end(), std::back_inserter(containedHash));
        std::cout << "Hash to str" << std::endl;
        auto containedWordB = val.at(1).as_blob();
        std:: cout << "Word getted" << std::endl;
        std::string containedWord;
        std::copy(containedWordB.begin(), containedWordB.end(), std::back_inserter(containedWord));
        std::cout << "containdrs getted" << std::endl;
        std::string hash(32, 'a');
        bool isOpOk = hashWithSalting(containedWord.c_str(), containedWord.size(), containedHash.c_str(), containedHash.size(), hash.data());
        
        if(isOpOk){
            std::cout << hash << std::endl;
            std::cout << data.hash << std::endl;
            isOk = (hash == data.hash);
        }
    }
    std::cout << "returning" << std::endl;
    return isOk;
}