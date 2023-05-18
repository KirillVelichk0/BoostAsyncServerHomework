#pragma once
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <string>
#include <memory>
#include <cstdint>
#include <vector>
#include <array>
#include <boost/asio/spawn.hpp>
#include "Rc4.hpp"
#include <optional>
namespace asio = boost::asio;
struct Client{
    asio::ip::tcp::socket socket;
    std::array<char, 4096> buf;
    std::vector<char> nonProcessedData;
    std::optional<Rc4Handler> cipherer;
};
using Socket_ptr = std::shared_ptr<Client>;
std::unique_ptr<Client> ConstructSocket(asio::io_service& sysService);
class MyWebSocketHandler{
public:
    static constexpr std::uint32_t maxMessageSize = 100000;

    void AsyncAccept(Socket_ptr sock, asio::ip::tcp::acceptor& acc, boost::system::error_code& code, asio::yield_context context);
    //гарантирует прием данных данного размера.
    std::string ReadForCount(Socket_ptr sock, std::uint32_t count, boost::system::error_code& ec,  asio::yield_context yield);
    //принимает некоторой объем данных, с заголовком в виде размера в формате BigEndian на 4 байта
    std::string ReadPackage(Socket_ptr sock, boost::system::error_code& ec, asio::yield_context yield);
    void SendPackage(Socket_ptr sock,std::string data, boost::system::error_code& ec, asio::yield_context yield);
};
