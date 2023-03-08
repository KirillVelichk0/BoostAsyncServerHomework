#include "MyWebSocket.h"
#include <boost/endian.hpp>
using namespace std::string_literals;
std::unique_ptr<Client> ConstructSocket(asio::io_service& sysService){
    return std::make_unique<Client>(Client{.socket = asio::ip::tcp::socket(sysService)});
}
MyWebSocket::MyWebSocket(asio::io_service &sysService) : socket(ConstructSocket(sysService)){}
MyWebSocket::~MyWebSocket() = default;
void MyWebSocket::AsyncAccept(asio::ip::tcp::acceptor &acc, boost::system::error_code &code, asio::yield_context context){
    acc.async_accept(this->socket->socket, context[code]);
}
// гарантирует прием данных данного размера.
std::string MyWebSocket::ReadForCount(std::uint32_t count, boost::system::error_code &ec, asio::yield_context yield) {
    if(!this->socket->socket.is_open()){
        ec = asio::error::not_connected;
        return ""s;
    }
    std::uint32_t curCount = 0;
    std::string result;
    auto client_socket = this->socket;
    if(!this->socket->nonProcessedData.empty()){
        std::uint32_t sz = client_socket->nonProcessedData.size();
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
// принимает некоторой объем данных, с заголовком в виде размера в формате BigEndian на 4 байта
std::string MyWebSocket::ReadPackage(boost::system::error_code &ec, asio::yield_context yield)  {
    auto szPackage = this->ReadForCount(4, ec, yield);
    if(ec.value() != 0){
        return ""s;
    }
    using NetworkInt32 = boost::endian::big_uint32_t;
    //считаем, что заголовок нам приходит в формате bigEndian32
    auto eSz = reinterpret_cast<NetworkInt32*>(szPackage.data());
    std::int32_t normalSize = *eSz;
    if(normalSize > MyWebSocket::maxMessageSize){
        ec = asio::error::message_size;
        return ""s;
    }
    return this->ReadForCount(normalSize, ec, yield);
}
void MyWebSocket::SendPackage(std::string data, boost::system::error_code &ec, asio::yield_context yield) {
    using NetworkInt32 = boost::endian::big_uint32_t;
    if(!this->socket->socket.is_open()){
        ec = asio::error::not_connected;
        return;
    }
    if(data.size() > MyWebSocket::maxMessageSize){
        ec = asio::error::message_size;
        return;
    }
    std::vector<char> buffer(4 + data.size());
    NetworkInt32 eSz = data.size();
    std::memcpy(buffer.data(), &eSz, 4);
    auto it = std::next(buffer.begin(), 4);
    decltype(auto) pos = *it;
    std::memcpy(&pos, data.data(), data.size());
    asio::async_write(this->socket->socket, asio::buffer(buffer), yield[ec]);

}