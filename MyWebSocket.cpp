#include "MyWebSocket.h"
#include <boost/endian.hpp>
#include <iostream>
using namespace std::string_literals;
std::unique_ptr<Client> ConstructSocket(asio::io_service &sysService)
{
    return std::make_unique<Client>(Client{.socket = asio::ip::tcp::socket(sysService)});
}

void MyWebSocketHandler::AsyncAccept(Socket_ptr sock, asio::ip::tcp::acceptor &acc, boost::system::error_code &code, asio::yield_context context)
{
    acc.async_accept(sock->socket, context[code]);
}
// гарантирует прием данных данного размера.
std::string MyWebSocketHandler::ReadForCount(Socket_ptr sock, std::uint32_t count, boost::system::error_code &ec, asio::yield_context yield)
{
    if (!sock->socket.is_open())
    {
        ec = asio::error::not_connected;
        return ""s;
    }
    std::uint32_t curCount = 0;
    std::vector<char> result(count);
    std::cout << "Hello from MyWebSock " << count << std::endl;
    std::cout << "nonProcessed in start " << sock->nonProcessedData.size() << std::endl;
    auto client_socket = sock;
    auto resIt = result.begin();
    if (!sock->nonProcessedData.empty())
    {
        std::cout << "PreprocData size " << sock->nonProcessedData.size() << std::endl;
        std::uint32_t sz = client_socket->nonProcessedData.size();
        auto bytesToRead = std::min(sz, count);
        auto sIt = client_socket->nonProcessedData.cbegin();
        curCount += bytesToRead;
        resIt = std::copy(sIt, std::next(sIt, bytesToRead), resIt);
        client_socket->nonProcessedData.erase(sIt, std::next(sIt, bytesToRead));
    }
    while (curCount < count)
    {
        std::cout << result.size() <<" " <<client_socket->nonProcessedData.size()<< std::endl;
        auto curReaded = client_socket->socket.async_read_some(asio::buffer(client_socket->buf), yield[ec]);
        std::cout << result.size() <<" " <<client_socket->nonProcessedData.size() << std::endl;
        std::cout << "curCount: " << curCount << " curReaded: " << curReaded << std::endl;
        if (ec)
        {
            std::cout << "Breaked" << std::endl;
            break;
        }
        if (curCount + curReaded > count)
        {
            auto nextDataCount = curCount + curReaded - count;
            resIt = std::copy(client_socket->buf.cbegin(), std::next(client_socket->buf.cbegin(), count - curCount), resIt);
            std::cout << result.size() <<" " <<client_socket->nonProcessedData.size()  << std::endl;
            std::copy(std::next(client_socket->buf.cbegin(), count - curCount), std::next(client_socket->buf.cbegin(), curReaded), std::back_inserter(client_socket->nonProcessedData));
            std::cout << "nonProcessed in end " << client_socket->nonProcessedData.size() << std::endl;
        }
        else
        {
            resIt = std::copy(client_socket->buf.cbegin(), std::next(client_socket->buf.cbegin(), curReaded), resIt);
            std::cout << "nonProcessed in not end " << client_socket->nonProcessedData.size() << std::endl;
        }
        curCount += curReaded;
    }
    for(int i = 0; i < client_socket->nonProcessedData.size(); i++){
        std::cout << client_socket->nonProcessedData[i];
    }
    std::cout <<std::endl;
    std::cout << "GoodBye from MyWebSock" << std::endl;
    std::string strResult = std::string(result.begin(), result.end());
    std::cout << "Result in MyWebSock " << strResult << std::endl;
    return strResult;
}
// принимает некоторой объем данных, с заголовком в виде размера в формате BigEndian на 4 байта
std::string MyWebSocketHandler::ReadPackage(Socket_ptr sock, boost::system::error_code &ec, asio::yield_context yield)
{
    std::cout << "Size in start ReadPack " << sock->nonProcessedData.size() << std::endl;
    auto szPackage = this->ReadForCount(sock, 4, ec, yield);
    if (ec.value() != 0)
    {
        return ""s;
    }
    using NetworkInt32 = boost::endian::big_uint32_t;
    // считаем, что заголовок нам приходит в формате bigEndian32
    auto eSz = reinterpret_cast<NetworkInt32 *>(szPackage.data());
    std::int32_t normalSize = *eSz;
    std::cout << "Package size is " << normalSize << std::endl;
    if (normalSize > MyWebSocketHandler::maxMessageSize)
    {
        ec = asio::error::message_size;
        return ""s;
    }
    std::cout << "NonProc size from send pack in front " << sock->nonProcessedData.size() << std::endl;
    return this->ReadForCount(sock, normalSize, ec, yield);
}
void MyWebSocketHandler::SendPackage(Socket_ptr sock, std::string data, boost::system::error_code &ec, asio::yield_context yield)
{
    using NetworkInt32 = boost::endian::big_uint32_t;
    if (!sock->socket.is_open())
    {
        ec = asio::error::not_connected;
        return;
    }
    if (data.size() > MyWebSocketHandler::maxMessageSize)
    {
        ec = asio::error::message_size;
        return;
    }
    std::int32_t sz = 4 + data.size();
    std::vector<char> buffer(4 + data.size());
    NetworkInt32 eSz = data.size();
    std::memcpy(buffer.data(), eSz.data(), 4);
    auto it = std::next(buffer.begin(), 4);
    decltype(auto) pos = *it;
    std::memcpy(&pos, data.data(), data.size());
    std::int32_t count = 0;
    while(count < sz){
        auto sendedBytes = asio::async_write(sock->socket, asio::buffer(buffer), yield[ec]);
        count += sendedBytes;
    }
    std::cout << "Sending data end in socket" << std::endl;
}