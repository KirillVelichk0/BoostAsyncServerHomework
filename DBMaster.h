#include <boost/asio/ssl.hpp>
#include <nlohmann/json.hpp>
#include <boost/asio.hpp>
#include <boost/mysql.hpp>
#include <memory>
#include "JsonMaster.h"
using Json = nlohmann::json;
class DBMaster{
private:

    boost::mysql::tcp_ssl_connection conn;
public:
    DBMaster(boost::asio::io_context& ctx, boost::asio::ssl::context& ssl_ctx);
    void SendRegistrDataToDB(const RegistrData& data);
    std::string GetWordFromDb(const KeyWordRequest& login);
    bool CompareSendedAuthDataWithDb(const AuthData &data);
};