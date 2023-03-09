#include "Server.h"
#include <algorithm>
#include <sstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <fstream>
#include <codecvt>
#include <locale>
#include <openssl/sha.h>
#include <string>
#include <iostream>
#include <cstdint>
#include <tuple>
#include <boost/coroutine/coroutine.hpp>
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
auto BuildMySqlSessionFromJsonConfig()
{
    auto data = GetMySqlDataFromJson();
    auto [host, port, user, password] = data;
    mysqlx::Session result(host.c_str(), port, user.c_str(), password.c_str());
    return result;
}
std::int32_t ParseVectorToInt32(const std::vector<char> &data)
{
    std::int32_t result;
    std::string str(data.begin(), data.end());
    std::istringstream istr(str);
    istr >> result;
    return result;
}
std::string utf8_to_string(const char *utf8str, const std::locale& loc)
{
    // UTF-8 to wstring
    std::wstring_convert<std::codecvt_utf8<wchar_t>> wconv;
    std::wstring wstr = wconv.from_bytes(utf8str);
    // wstring to string
    std::vector<char> buf(wstr.size());
    std::use_facet<std::ctype<wchar_t>>(loc).narrow(wstr.data(), wstr.data() + wstr.size(), '?', buf.data());
    return std::string(buf.data(), buf.size());
}
void ServerDemon::SendDataToDB(const std::pair<std::string, std::string> &data)
{
    using namespace mysqlx;
    std::string hashed(32, 'a');
    std::cout << "startSending" << std::endl;
    SHA256(reinterpret_cast<const unsigned char *>(data.second.data()), data.second.size(), reinterpret_cast<unsigned char *>(hashed.data()));
    std::cout << "SHA is ok" << std::endl;
    Schema db = sess.getSchema("IBSystems");
    Table myColl = db.getTable("passcont");
    mysqlx::string login = data.first;
    mysqlx::bytes finalHash = hashed.c_str();

    //auto login = utf8_to_string(data.first.c_str(), std::locale("en_US.UTF-8"));
    //hashed = utf8_to_string(hashed.c_str(), std::locale("en_US.UTF-8"));
    std::cout << "Finally" << std::endl;
    try{
        std::cout << data.first.c_str() << " " << hashed.c_str() << std::endl;
        myColl.insert("uid", "login", "passH").values(0, login, finalHash).execute();
    } catch (std::exception& err){
        std::cout << "Uncorrect locale" << std::endl;
    }
    
}
std::pair<std::string, std::string> ServerDemon::GetAuthDataForRegist(JsonMq &jsonMqSession, boost::system::error_code &ec, asio::yield_context yield)
{
    std::pair<std::string, std::string> result;
    try{
    auto jsonData = jsonMqSession.GetJson(ec, yield);
    if (ec.value() == 0)
    {
        std::cout << "Json getted" << std::endl;
        auto login = jsonData.find("username");
        auto password = jsonData.find("password");
        std::cout << "finded" << std::endl;
        if (login == jsonData.cend() || password == jsonData.cend())
        {
            ec = asio::error::invalid_argument;
            std::cout << "bad" << std::endl;
        }
        else
        {
            std::cout << "Json linking" << std::endl;
            result.first = login->get<std::string>();
            result.second = password->get<std::string>();
            std::cout << "Json linked" << std::endl;
            std::cout << result.first << " " << result.second << std::endl;
        }
    }
    return result;
    } catch(std::exception& ex){
        ec = asio::error::invalid_argument;
        return result;
    }
}
void ServerDemon::do_clientSession(JsonMq &jsonMqSession, boost::system::error_code &ec)
{
    std::pair<std::string, std::string> authData;
    auto self = shared_from_this();
    auto dataForSpawnFactory = [self, &ec, &jsonMqSession](auto result)
    {
        return [self, &ec, &jsonMqSession, result](asio::yield_context yield) mutable
        {
            std::cout << "ClientSession started" << std::endl;
            try{
                auto preRes = self->GetAuthDataForRegist(jsonMqSession, ec, yield);
                std::cout << "normal preRes" << std::endl;
                result = preRes;
                std::cout<< "what a hell" << std::endl;
            } catch (std::exception& e){
                std::cout << "wtf" << std::endl;
            }
            if(ec.value() == 0){
                std::cout << "Getting data ok" << std::endl;
                self->SendDataToDB(result);
            }
            else{
                std::cout << "Getting data bed" << std::endl;
                std::cout << ec.value() << std::endl;
            }
        };
    };
    asio::spawn(self->sysService, dataForSpawnFactory(authData));
}
void ServerDemon::do_accept(JsonMq &jsonMqSession, boost::system::error_code &ec, asio::yield_context yield)
{
    auto self = shared_from_this();
    jsonMqSession = JsonMq(self->sysService, self->acc, ec, yield);
    if(ec.value() == 0){
       std::cout << "Connection Ok" << std::endl;
    }else{
        std::cout << "Bad conn " << ec.message()<< std::endl;
    }
}

ServerDemon::ServerDemon() : ep(asio::ip::tcp::v4(), 2001), acc(sysService, ep), sess(BuildMySqlSessionFromJsonConfig()) {}
void ServerDemon::RunThread(asio::yield_context yield)
{
    try
    {
        auto self = shared_from_this();
        while (true)
        {
            JsonMq jsonMqSession;
            boost::system::error_code code;
            self->do_accept(jsonMqSession, code, yield);
            if(code.value() == 0){
                do_clientSession(jsonMqSession, code);
            }
            boost::this_thread::interruption_point();
        }
    }
    catch (boost::thread_interrupted &inter)
    {
        return;
    }
}
std::shared_ptr<ServerDemon> ServerDemon::GetPtr(){
    return shared_from_this();
}

std::shared_ptr<ServerDemon> ServerDemon::Create(){
    return std::shared_ptr<ServerDemon>(new ServerDemon(), ServerDeleter<ServerDemon>());
}
void ServerDemon::RunDemon()
{
    auto self = shared_from_this();
    std::cout << "Get self" << std::endl;
    auto runYield = [self](asio::yield_context yield)
    {
        self->RunThread(yield);
    };
    auto run = [&runYield, self]()
    {
        asio::spawn(self->sysService, runYield);
        self->sysService.run();
    };
    for (int i = 0; i < 1; i++)
    {
        self->threads.create_thread(run);
    }
    std::string answer;
    std::cout << "Server started" << std::endl;
    while (true)
    {
        std::cin >> answer;
        if (answer == "exit")
        {
            std::cout << "Bye" << std::endl;
            break;
        }
    }
}
