#pragma once
#include <boost/asio.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/thread.hpp>
#include <boost/coroutine/coroutine.hpp>
#include <array>
#include <vector>
#include <memory>
#include <cstdint>
#include "JsonMq.h"
#include <optional>
#include "DBMaster.h"
namespace asio = boost::asio;
template<class T>
using MyRf = std::reference_wrapper<T>;
class ServerDemon;
using CallableReaction = std::function<void(MyRf<JsonMq>, std::weak_ptr<ServerDemon>, MyRf<Json>, MyRf<boost::system::error_code>, asio::yield_context)>;
template <class T>
struct ServerDeleter{
    void operator ()( T* p)
  { 
    p->context.stop();
    p->threads.interrupt_all();
    p->threads.join_all();
  }
};
class ServerDemon : public std::enable_shared_from_this<ServerDemon>{
private:
    friend struct ServerDeleter<ServerDemon>;
    boost::asio::io_context context;
    boost::asio::ssl::context ssl_ctx;
    asio::ip::tcp::endpoint ep;
    asio::ip::tcp::acceptor acc;
    boost::thread_group threads;
    DBMaster master;

private:
    ServerDemon();
    //функция не должна быть блокируемой или являться корутиной.
    //Она просто порождает контекст дальнейшей работы
    void do_clientSession(JsonMq&& jsonMqSession);
    void do_accept(JsonMq& jsonMqSession, boost::system::error_code&, asio::yield_context yield);
    void RunThread(asio::yield_context yield);
public:
    auto& GetDbSession();
    static std::shared_ptr<ServerDemon> Create();
    std::shared_ptr<ServerDemon> GetPtr();
    void RunDemon();
    std::optional<CallableReaction> GetReaction(const std::string& requestType);
};
