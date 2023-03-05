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
auto GetMySqlDataFromJson(){
    std::tuple<std::string, std::int32_t, std::string, std::string> result;
    using json = nlohmann::json;
    std::ifstream istr("../configs/mySqlDbConfig.json");
    std::cout << "startParsing" << std::endl;
    json sqlConfJson = json::parse(istr);
    std::cout << "first config parsed" << std::endl;
    istr.close();
    std::get<0>(result) =  sqlConfJson["hostName"].get<std::string>();
    std::get<1>(result) = sqlConfJson["port"].get<std::int32_t>();
    std::get<2>(result) = sqlConfJson["user"].get<std::string>();
    std::string pathToPassword = sqlConfJson["pathToPasswordFile"].get<std::string>();
    istr.open(pathToPassword);
    json dbPassword = json::parse(istr);
    istr.close();
    std::get<3>(result) = dbPassword["password"].get<std::string>();
    return result;
}
auto BuildMySqlSessionFromJsonConfig(){
    auto data = GetMySqlDataFromJson();
    auto [host, port, user, password] = data;
    mysqlx::Session result(host.c_str(), port, user.c_str(), password.c_str());
    return result;
}
std::int32_t ParseVectorToInt32(const std::vector<char>& data){
    std::int32_t result;
    std::string str(data.begin(), data.end());
    std::istringstream istr(str);
    istr >> result;
    return result;
}
void ServerDemon::SendDataToDB(const std::pair<std::vector<char>, std::vector<char>>& data){
    using namespace mysqlx;
    std::string hashed(32, 'a');
    SHA256(reinterpret_cast<const unsigned char*>(data.second.data()), data.second.size(), reinterpret_cast<unsigned char*>(hashed.data()));
    Schema db = sess.getSchema("IBSystems");
    Table myColl = db.getTable("passcont");
    std::string login(data.first.cbegin(), data.first.cend());
    myColl.insert("uid", "login", "passH").values("NULL", login, hashed).execute();
}
std::int32_t ServerDemon::AsyncRead(Socket_ptr client_socket, boost::system::error_code& ec, std::vector<char>& data, asio::yield_context yield){
    return client_socket->socket.async_read_some(asio::buffer(client_socket->buf), yield[ec]);
}
std::pair<std::vector<char>, std::vector<char>> ServerDemon::GetAuthDataForRegist(Socket_ptr client_socket, boost::system::error_code& ec, asio::yield_context yield){
    std::pair<std::vector<char>, std::vector<char>> result;
    auto dataForSpawnFactory = [this, &ec, client_socket](auto count, auto& result){
        return [this, &ec, client_socket, count, &result](asio::yield_context yield) mutable{
            result = this->ReadForCount(client_socket, ec, count, yield);
        };
    };
    std::vector<char> loginLen;
    asio::spawn(this->sysService, dataForSpawnFactory(4, loginLen));
    if(ec){
        return result;
    }
    std::int32_t loginLenParsed = ParseVectorToInt32(loginLen);
    asio::spawn(this->sysService, dataForSpawnFactory(loginLenParsed, result.first));
    if(ec){
        return result;
    }
    std::vector<char> passwordLen;
    asio::spawn(this->sysService, dataForSpawnFactory(4, passwordLen));
    if(ec){
        return result;
    }
    auto passLenParsed = ParseVectorToInt32(passwordLen);
    asio::spawn(this->sysService, dataForSpawnFactory(passLenParsed, result.second));
    return result;

}
std::vector<char> ServerDemon::ReadForCount(Socket_ptr client_socket, boost::system::error_code& ec, std::int32_t count,  asio::yield_context yield){
    std::int32_t curCount = 0;
    std::vector<char> result;
    if(!client_socket->nonProcessedData.empty()){
        std::int32_t sz = client_socket->nonProcessedData.size();
        auto bytesToRead = std::min(sz, count);
        auto sIt =client_socket->nonProcessedData.cbegin();
        curCount+=bytesToRead;
        std::copy(sIt, std::next(sIt, bytesToRead), std::back_inserter(result));
        client_socket->nonProcessedData.erase(sIt, std::next(sIt, bytesToRead));
    }
    while(curCount < count){
        auto curReaded = client_socket->socket.async_read_some(asio::buffer(client_socket->buf), yield[ec]);
        if(ec){
            break;
        }
        if(curCount + curReaded > count){
            std::copy(client_socket->buf.cbegin(), std::next(client_socket->buf.cbegin(), curReaded - (curCount- count)), std::back_inserter(result));
            std::copy(client_socket->buf.cbegin(), std::next(client_socket->buf.cbegin(), (curCount- count)), std::back_inserter(client_socket->nonProcessedData));
        }
        else{
            std::copy(client_socket->buf.cbegin(), std::next(client_socket->buf.cbegin(), curReaded), std::back_inserter(result));
        }
        curCount += curReaded;
    }

    return result;
}
void ServerDemon::do_clientSession(Socket_ptr client_socket, boost::system::error_code& ec){
    std::pair<std::vector<char>, std::vector<char>> authData;
    auto dataForSpawnFactory = [this, &ec, client_socket](auto& result){
        return [this, &ec, client_socket, &result](asio::yield_context yield) mutable{
            result = this->GetAuthDataForRegist(client_socket, ec, yield);
            this->SendDataToDB(result);
        };
    };
    asio::spawn(this->sysService, dataForSpawnFactory(authData));
}
void ServerDemon::do_accept(Socket_ptr client_socket, boost::system::error_code& ec, asio::yield_context yield){
    this->acc.async_accept(client_socket->socket, yield[ec]);
}

ServerDemon::ServerDemon() : ep(asio::ip::tcp::v4(), 2001), acc(sysService, ep), sess(BuildMySqlSessionFromJsonConfig()){}
void ServerDemon::RunThread(asio::yield_context yield){
    try {
        while (true) {
            Socket_ptr sock = std::make_shared<Client>(Client{asio::ip::tcp::socket(this->sysService)});
            boost::system::error_code code;
            this->do_accept(sock, code, yield);
            do_clientSession(sock, code);
            boost::this_thread::interruption_point();
        }
    } catch(boost::thread_interrupted& inter){
        return;
    }
}
ServerDemon::~ServerDemon(){
    this->threads.interrupt_all();
    this->sysService.stop();
    this->threads.join_all();
}
void ServerDemon::RunDemon(){
    auto runYield = [this](asio::yield_context yield){
        this->RunThread(yield);
    };
    auto run = [&runYield, this](){
        asio::spawn(this->sysService, runYield);
        this->sysService.run();
    };
    for(int i = 0; i < 8; i++){
        this->threads.create_thread(run);
    }
    std::string answer;
    std::cout << "Server started" << std::endl;
    while(true){
        std::cin >> answer;
        if(answer == "exit"){
            std::cout << "Bye" << std::endl;
            break;
        }
    }
}
