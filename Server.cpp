#include "Server.h"
#include <algorithm>
#include <sstream>
#include <openssl/md5.h>
#include<string>
using namespace std::string_literals;
std::int32_t ParseVectorToInt32(const std::vector<char>& data){
    std::int32_t result;
    std::string str(data.begin(), data.end());
    std::istringstream istr(str);
    istr >> result;
    return result;
}
void ServerDemon::SendDataToDB(const std::pair<std::vector<char>, std::vector<char>>& data){
    using namespace mysqlx;
    std::string hashed(16, 'a');
    MD5(reinterpret_cast<const unsigned char*>(data.second.data()), data.second.size(), reinterpret_cast<unsigned char*>(hashed.data()));
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
std::vector<char> ServerDemon::ReadForCount(Socket_ptr client_socket, boost::system::error_code& ec, std::int32_t count, asio::yield_context yield){
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

ServerDemon::ServerDemon() : ep(asio::ip::tcp::v4(), 2001), acc(sysService, ep), sess("localhost", 33060, "root", "12345434"){}
void ServerDemon::RunThread(asio::yield_context yield){
    try {
        while (true) {
            Socket_ptr sock = std::make_shared<Client>(Client{asio::ip::tcp::socket(this->sysService)});
            boost::system::error_code code;
            this->do_accept(sock, code, yield);
            do_clientSession(sock, code);
            boost::this_thread::sleep_for(boost::chrono::seconds(0));
        }
    } catch(boost::thread_interrupted& inter){
        return;
    }
}
ServerDemon::~ServerDemon(){
    this->threads.interrupt_all();
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
}
