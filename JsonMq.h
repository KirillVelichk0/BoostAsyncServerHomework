#pragma once
#include <memory>
#include <boost/asio.hpp>
#include <boost/asio/spawn.hpp>
#include <nlohmann/json.hpp>
namespace asio = boost::asio;
using Json = nlohmann::json;
class JsonMq{
public:
    struct Impl;
private:
    std::unique_ptr<Impl> impl;
public:
    JsonMq();
    JsonMq(asio::io_service& sysService, asio::ip::tcp::acceptor& acc, boost::system::error_code& ec, asio::yield_context yield);
    ~JsonMq();
    JsonMq(const JsonMq& another);
    JsonMq(JsonMq&& another);
    JsonMq& operator=(const JsonMq& another);
    JsonMq& operator=(JsonMq&& another);
    void SetKey(std::int32_t key);
    Json GetJson(boost::system::error_code& ec, asio::yield_context yield) const;
    void SendJson(const Json& jsonData, boost::system::error_code& ec, asio::yield_context yield) const;

};