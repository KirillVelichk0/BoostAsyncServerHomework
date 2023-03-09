#include "JsonMq.h"
#include "MyWebSocket.h"
#include <sstream>
#include <iostream>
struct JsonMq::Impl
{
    Socket_ptr socket;

};
std::unique_ptr<JsonMq::Impl> CreateMyWebSocket(asio::io_service &sysService){
    return std::make_unique<JsonMq::Impl>(JsonMq::Impl{.socket = std::make_shared<Client>(Client{.socket = asio::ip::tcp::socket(sysService)})});
}
auto CopyMyWebSocket(Socket_ptr sock){
    return std::make_unique<JsonMq::Impl>(JsonMq::Impl{.socket = sock});
}
JsonMq::JsonMq() = default;
JsonMq::JsonMq(asio::io_service &sysService,asio::ip::tcp::acceptor& acc, boost::system::error_code& ec, asio::yield_context yield) : impl(CreateMyWebSocket(sysService)){
    MyWebSocketHandler handler;
    handler.AsyncAccept(this->impl->socket, acc, ec, yield);
}
JsonMq::~JsonMq() = default;
JsonMq::JsonMq(const JsonMq &another) : impl(CopyMyWebSocket(another.impl->socket)){}
JsonMq::JsonMq(JsonMq &&another) = default;
JsonMq& JsonMq::operator=(const JsonMq &another){
    this->impl = CopyMyWebSocket(another.impl->socket);
    return *this;
}
JsonMq& JsonMq::operator=(JsonMq &&another) = default;
Json JsonMq::GetJson(boost::system::error_code &ec, asio::yield_context yield) const {
    Json result;
    MyWebSocketHandler handler;
    auto strJson = handler.ReadPackage(this->impl->socket, ec, yield);
    if(ec.value()== 0){
        std::cout << "Str is normal. Called from GetJson" << std::endl;
        std::istringstream isstr(strJson);
        try{
        isstr>> result;
        } catch (std::exception& ex){
            ec = asio::error::invalid_argument;
            std::cout << "InvalidExep" << std::endl;
            result = Json();
            return result;
        }
        if(isstr.fail()){
            ec = asio::error::invalid_argument;
            std::cout << "InvalidEofBit" << std::endl;
            result = Json();
        }
        else{
            std::cout << result << std::endl;
        }
    }
    std::cout << "Hello from GetJson" << std::endl;
    return result;
}
void JsonMq::SendJson(const Json &jsonData, boost::system::error_code &ec, asio::yield_context yield) const{
    std::string jsonStr = jsonData.dump();
    MyWebSocketHandler handler;
    handler.SendPackage(this->impl->socket, jsonStr, ec, yield);
}