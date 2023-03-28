#include "MyWebSocket.h"
#include <boost/endian.hpp>
#include <iostream>
#include <openssl/rsa.h>
#include <openssl/rand.h>
#include <openssl/pem.h>
#include <openssl/sha.h>
#include <tuple>
using namespace std::string_literals;
//S, Nonce, OpenKey
using AuthTriplet = std::tuple<std::string,std::string, std::string>;

std::string RsaPriEncrypt(const std::string &clear_text, std::string &pri_key)
{
	std::string encrypt_text;
	BIO *keybio = BIO_new_mem_buf((unsigned char *)pri_key.c_str(), -1);
	RSA* rsa = RSA_new();
	rsa = PEM_read_bio_RSAPrivateKey(keybio, &rsa, NULL, NULL);
	if (!rsa)
	{
		BIO_free_all(keybio);
		return std::string("");
	}
 
	 // Get the maximum length of data that RSA can process at a time
	int len = RSA_size(rsa);
 
	 // Apply for memory: store encrypted ciphertext data
	char *text = new char[len + 1];
	memset(text, 0, len + 1);
 
	 // Encrypt the data with a private key (the return value is the length of the encrypted data)
	int ret = RSA_private_encrypt(clear_text.length(), (const unsigned char*)clear_text.c_str(), (unsigned char*)text, rsa, RSA_PKCS1_PADDING);
	if (ret >= 0) {
		encrypt_text = std::string(text, ret);
	}
 
	 // release memory  
	free(text);
	BIO_free_all(keybio);
	RSA_free(rsa);
 
	return encrypt_text;
}
void GenerateRSAKey(std::string & out_pub_key, std::string & out_pri_key)
{
	size_t pri_len = 0; // Private key length
	size_t pub_len = 0; // public key length
	 char *pri_key = nullptr; // private key
	 char *pub_key = nullptr; // public key
 
	 // Generate key pair
	RSA *keypair = RSA_generate_key(2048, RSA_3, NULL, NULL);
 
	BIO *pri = BIO_new(BIO_s_mem());
	BIO *pub = BIO_new(BIO_s_mem());
 
         // Generate private key
	PEM_write_bio_RSAPrivateKey(pri, keypair, NULL, NULL, 0, NULL, NULL);
         // Note------Generate the public key in the first format
    //PEM_write_bio_RSAPublicKey(pub, keypair);
         // Note------Generate the public key in the second format (this is used in the code here)
	PEM_write_bio_RSA_PUBKEY(pub, keypair);

    pri_len = BIO_pending(pri);
	pub_len = BIO_pending(pub);
 
	 // The key pair reads the string  
	pri_key = (char *)malloc(pri_len + 1);
	pub_key = (char *)malloc(pub_len + 1);

    BIO_read(pri, pri_key, pri_len);
	BIO_read(pub, pub_key, pub_len);
 
	pri_key[pri_len] = '\0';
	pub_key[pub_len] = '\0';
 
	out_pub_key = pub_key;
	out_pri_key = pri_key;

    RSA_free(keypair);
	BIO_free_all(pub);
	BIO_free_all(pri);
 
	free(pri_key);
	free(pub_key);
}

auto GenTriplet(){
    AuthTriplet result;
    auto& [s, nonce, openKey] = result;
    unsigned char nonceBuf[32];
    RAND_bytes(nonceBuf, 32);
    nonce = std::string(reinterpret_cast<char*>(nonceBuf), 32);
    std::string closeKey;

    GenerateRSAKey(openKey, closeKey);
    unsigned char hash_buf[32];
    SHA256(nonceBuf, 32, hash_buf);
    s = RsaPriEncrypt(std::string((char*)hash_buf, 32), closeKey);
    return result;
}
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