#include "JsonMq.h"
#include "MyWebSocket.h"
#include <sstream>
struct JsonMq::Impl
{
    MyWebSocket socket;
};
std::unique_ptr<JsonMq::Impl> CreateMyWebSocket(asio::io_service &sysService){
    return std::make_unique<JsonMq::Impl>(JsonMq::Impl{.socket = MyWebSocket(sysService)});
}
auto CopyMyWebSocket(const MyWebSocket& sock){
    return std::make_unique<JsonMq::Impl>(JsonMq::Impl{.socket = sock});
}
JsonMq::JsonMq() = default;
JsonMq::JsonMq(asio::io_service &sysService,asio::ip::tcp::acceptor& acc, boost::system::error_code& ec, asio::yield_context yield) : impl(CreateMyWebSocket(sysService)){
    this->impl->socket.AsyncAccept(acc, ec, yield);
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
    auto strJson = this->impl->socket.ReadPackage(ec, yield);
    if(ec.value()== 0){
        std::istringstream isstr(strJson);
        isstr>> result;
        if(isstr.fail()){
            ec = asio::error::invalid_argument;
        }
    }
    return result;
}
void JsonMq::SendJson(const Json &jsonData, boost::system::error_code &ec, asio::yield_context yield) const{
    std::string jsonStr = jsonData.dump();
    this->impl->socket.SendPackage(jsonStr, ec, yield);
}