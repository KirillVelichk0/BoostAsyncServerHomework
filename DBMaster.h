#include <mysqlx/xdevapi.h>
#include <nlohmann/json.hpp>
#include <memory>
class DBMaster{
private:
    mysqlx::Session sess;
public:
    DBMaster();
    void SendRegistrDataToDB(const std::pair<std::string, std::string> &data);
    std::string GetWordFromDb(std::string login);
    bool CompareSendedAuthDataWithDb(const std::pair<std::string, std::string> &data);
};