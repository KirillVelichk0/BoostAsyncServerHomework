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
template <class T>
struct ServerDeleter{
    void operator ()( T* p)
  { 
    p->sysService.stop();
    p->threads.interrupt_all();
    p->threads.join_all();
  }
};
class ServerDemon : public std::enable_shared_from_this<ServerDemon>{
private:
    friend struct ServerDeleter<ServerDemon>;
    asio::io_service sysService;
    asio::ip::tcp::endpoint ep;
    asio::ip::tcp::acceptor acc;
    boost::thread_group threads;
    mysqlx::Session sess;

private:
    ServerDemon();
    //функция не должна быть блокируемой или являться корутиной.
    //Она просто порождает контекст дальнейшей работы
    void do_clientSession(JsonMq& jsonMqSession, boost::system::error_code&);
    void do_accept(JsonMq& jsonMqSession, boost::system::error_code&, asio::yield_context yield);
    std::pair<std::string, std::string> GetAuthDataForRegist(JsonMq& jsonMqSession, boost::system::error_code&, asio::yield_context yield);
    void RunThread(asio::yield_context yield);
    void SendDataToDB(const std::pair<std::string, std::string>& data);
public:
    static std::shared_ptr<ServerDemon> Create();
    std::shared_ptr<ServerDemon> GetPtr();
    void RunDemon();
};
