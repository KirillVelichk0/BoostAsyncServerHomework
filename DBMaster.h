#include <mysqlx/xdevapi.h>
#include <nlohmann/json.hpp>
#include <memory>
#include "JsonMaster.h"
using Json = nlohmann::json;
class DBMaster{
private:
    mysqlx::Session sess;
public:
    DBMaster();
    void SendRegistrDataToDB(const RegistrData& data);
    std::string GetWordFromDb(const KeyWordRequest& login);
    bool CompareSendedAuthDataWithDb(const AuthData &data);
};