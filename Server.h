#pragma once
#include <boost/asio.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/thread.hpp>
#include <boost/coroutine/coroutine.hpp>
#include <array>
#include <vector>
#include <memory>
#include <cstdint>
#include "JsonMq.h"
#include <mysqlx/xdevapi.h>
namespace asio = boost::asio;
class ServerDemon{
private:
    asio::io_service sysService;
    asio::ip::tcp::endpoint ep;
    asio::ip::tcp::acceptor acc;
    boost::thread_group threads;
    mysqlx::Session sess;
private:
    //функция не должна быть блокируемой или являться корутиной.
    //Она просто порождает контекст дальнейшей работы
    void do_clientSession(JsonMq& jsonMqSession, boost::system::error_code&);
    void do_accept(JsonMq& jsonMqSession, boost::system::error_code&, asio::yield_context yield);
    std::pair<std::string, std::string> GetAuthDataForRegist(JsonMq& jsonMqSession, boost::system::error_code&, asio::yield_context yield);
    void RunThread(asio::yield_context yield);
    void SendDataToDB(const std::pair<std::string, std::string>& data);
public:
    ServerDemon();
    ~ServerDemon();
    void RunDemon();
};
