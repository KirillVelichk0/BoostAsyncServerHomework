#include "Server.h"
#include <algorithm>
#include <sstream>
#include <nlohmann/json.hpp>
#include <fstream>
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
void ServerDemon::SendDataToDB(const std::pair<std::string, std::string> &data)
{
    using namespace mysqlx;
    std::string hashed(32, 'a');
    SHA256(reinterpret_cast<const unsigned char *>(data.second.data()), data.second.size(), reinterpret_cast<unsigned char *>(hashed.data()));
    Schema db = sess.getSchema("IBSystems");
    Table myColl = db.getTable("passcont");
    myColl.insert("uid", "login", "passH").values("NULL", data.first, hashed).execute();
}
std::pair<std::string, std::string> ServerDemon::GetAuthDataForRegist(JsonMq &jsonMqSession, boost::system::error_code &ec, asio::yield_context yield)
{
    std::pair<std::string, std::string> result;
    auto jsonData = jsonMqSession.GetJson(ec, yield);
    if (ec.value() == 0)
    {
        auto login = jsonData.find("login");
        auto password = jsonData.find("password");
        if (login == jsonData.cend() || password == jsonData.cend())
        {
            ec = asio::error::invalid_argument;
        }
        else
        {
            result.first = login->get<std::string>();
            result.second = password->get<std::string>();
        }
    }
    return result;
}
void ServerDemon::do_clientSession(JsonMq &jsonMqSession, boost::system::error_code &ec)
{
    std::pair<std::string, std::string> authData;
    auto dataForSpawnFactory = [this, &ec, &jsonMqSession](auto &result)
    {
        return [this, &ec, &jsonMqSession, &result](asio::yield_context yield) mutable
        {
            result = this->GetAuthDataForRegist(jsonMqSession, ec, yield);
            this->SendDataToDB(result);
        };
    };
    asio::spawn(this->sysService, dataForSpawnFactory(authData));
}
void ServerDemon::do_accept(JsonMq &jsonMqSession, boost::system::error_code &ec, asio::yield_context yield)
{
    jsonMqSession = JsonMq(this->sysService, this->acc, ec, yield);
}

ServerDemon::ServerDemon() : ep(asio::ip::tcp::v4(), 2001), acc(sysService, ep), sess(BuildMySqlSessionFromJsonConfig()) {}
void ServerDemon::RunThread(asio::yield_context yield)
{
    try
    {
        while (true)
        {
            JsonMq jsonMqSession;
            boost::system::error_code code;
            this->do_accept(jsonMqSession, code, yield);
            do_clientSession(jsonMqSession, code);
            boost::this_thread::interruption_point();
        }
    }
    catch (boost::thread_interrupted &inter)
    {
        return;
    }
}
ServerDemon::~ServerDemon()
{
    this->threads.interrupt_all();
    this->sysService.stop();
    this->threads.join_all();
}
void ServerDemon::RunDemon()
{
    auto runYield = [this](asio::yield_context yield)
    {
        this->RunThread(yield);
    };
    auto run = [&runYield, this]()
    {
        asio::spawn(this->sysService, runYield);
        this->sysService.run();
    };
    for (int i = 0; i < 8; i++)
    {
        this->threads.create_thread(run);
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
