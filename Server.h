#pragma once
#include <boost/asio.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/thread.hpp>
#include <boost/coroutine/coroutine.hpp>
#include <array>
#include <vector>
#include <memory>
#include <cstdint>
#include <mysqlx/xdevapi.h>
namespace asio = boost::asio;
struct Client{
    asio::ip::tcp::socket socket;
    std::array<char, 4096> buf;
    std::vector<char> nonProcessedData;
};
using Socket_ptr = std::shared_ptr<Client>;
class ServerDemon{
private:
    asio::io_service sysService;
    asio::ip::tcp::endpoint ep;
    asio::ip::tcp::acceptor acc;
    boost::thread_group threads;
    mysqlx::Session sess;
private:
    std::vector<char> ReadForCount(Socket_ptr client_socket, boost::system::error_code&, std::int32_t count, asio::yield_context yield);
    std::int32_t AsyncRead(Socket_ptr client_socket, boost::system::error_code&, std::vector<char>& data, asio::yield_context yield);
    //функция не должна быть блокируемой или являться корутиной.
    //Она просто порождает контекст дальнейшей работы
    void do_clientSession(Socket_ptr tcp_socket, boost::system::error_code&);
    void do_accept(Socket_ptr client_socket, boost::system::error_code&, asio::yield_context yield);
    std::pair<std::vector<char>, std::vector<char>> GetAuthDataForRegist(Socket_ptr client_socket, boost::system::error_code&, asio::yield_context yield);
    void RunThread(asio::yield_context yield);
    void SendDataToDB(const std::pair<std::vector<char>, std::vector<char>>& data);
public:
    ServerDemon();
    ~ServerDemon();
    void RunDemon();
};
