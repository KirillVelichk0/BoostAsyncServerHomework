#pragma once
#include <boost/asio.hpp>
#include <string>
#include <memory>
#include "JsonMq.h"
#include <cstdint>
#include <vector>
#include <array>
#include <boost/asio/spawn.hpp>
namespace asio = boost::asio;
struct Client{
    asio::ip::tcp::socket socket;
    std::array<char, 4096> buf;
    std::vector<char> nonProcessedData;
};
using Socket_ptr = std::shared_ptr<Client>;
std::unique_ptr<Client> ConstructSocket(asio::io_service& sysService);
class MyWebSocket{
public:
    static constexpr std::uint32_t maxMessageSize = 100000;
private:
    Socket_ptr socket;
public:
    MyWebSocket(asio::io_service& sysService);
    ~MyWebSocket();
    void AsyncAccept(asio::ip::tcp::acceptor& acc, boost::system::error_code& code, asio::yield_context context);
    //гарантирует прием данных данного размера.
    std::string ReadForCount(std::uint32_t count, boost::system::error_code& ec,  asio::yield_context yield);
    //принимает некоторой объем данных, с заголовком в виде размера в формате BigEndian на 4 байта
    std::string ReadPackage(boost::system::error_code& ec, asio::yield_context yield);
    void SendPackage(std::string data, boost::system::error_code& ec, asio::yield_context yield);
};
