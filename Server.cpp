#include "Server.h"
#include <algorithm>
#include <sstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <fstream>
#include <codecvt>
#include <locale>
#include <openssl/sha.h>
#include <string>
#include <iostream>
#include <cstdint>
#include <tuple>
#include <boost/coroutine/coroutine.hpp>
using namespace std::string_literals;
std::pair<std::string, std::string> ServerDemon::GetAuthDataForRegist(Json &jsonData, boost::system::error_code &ec, asio::yield_context yield)
{
    std::pair<std::string, std::string> result;
    try
    {
        auto login = jsonData.find("username");
        auto password = jsonData.find("password");
        std::cout << "finded" << std::endl;
        if (login == jsonData.cend() || password == jsonData.cend())
        {
            ec = asio::error::invalid_argument;
            std::cout << "bad" << std::endl;
        }
        else
        {
            std::cout << "Json linking" << std::endl;
            result.first = login->get<std::string>();
            result.second = password->get<std::string>();
            std::cout << "Json linked" << std::endl;
            std::cout << result.first << " " << result.second << std::endl;
        }
        return result;
    }
    catch (std::exception &ex)
    {
        ec = asio::error::invalid_argument;
        return result;
    }
}
auto &ServerDemon::GetDbSession()
{
    return this->master;
}
std::map<std::string, CallableReaction> ConstructReacions()
{
    std::map<std::string, CallableReaction> result;
    {
        auto SendRegistDataToDb = [](MyRf<JsonMq> jsonSession, std::weak_ptr<ServerDemon> serverSession, MyRf<Json> data, MyRf<boost::system::error_code> ec, asio::yield_context yield)
        {
            auto sharedContext = serverSession.lock();
            if (sharedContext != nullptr)
            {
                auto registrData = sharedContext->GetAuthDataForRegist(data.get(), ec, yield);
                auto &dbSession = sharedContext->GetDbSession();
                dbSession.SendRegistrDataToDB(registrData);
            }
        };
        result.insert(std::make_pair("RegistrationRequest", std::move(SendRegistDataToDb)));
    }
    {
        auto SendWordToClient = [](MyRf<JsonMq> jsonSession, std::weak_ptr<ServerDemon> serverSession, MyRf<Json> data, MyRf<boost::system::error_code> ec, asio::yield_context yield)
        {
            auto sharedContext = serverSession.lock();
            if (sharedContext != nullptr)
            {
                std::pair<std::string, std::string> result;
                try
                {
                    auto login = data.get().find("username");
                    if (login == data.get().cend())
                    {
                        ec.get() = asio::error::invalid_argument;
                        std::cout << "bad" << std::endl;
                    }
                    else
                    {
                        auto word = sharedContext->GetDbSession().GetWordFromDb(login->get<std::string>());
                        Json dataToSend;
                        dataToSend["KeyWord"] = word;
                        jsonSession.get().SendJson(dataToSend, ec.get(), yield);
                    }
                }
                catch (std::exception &ex)
                {
                    ec.get() = asio::error::invalid_argument;
                }
            }
        };
        result.insert(std::make_pair("GetKeyWordRequest", std::move(SendWordToClient)));
    }
    {
        auto CompareHashs = [](MyRf<JsonMq> jsonSession, std::weak_ptr<ServerDemon> serverSession, MyRf<Json> data, MyRf<boost::system::error_code> ec, asio::yield_context yield){
            auto sharedContext = serverSession.lock();
            if (sharedContext != nullptr){
                try
                {
                    auto login = data.get().find("username");
                    auto hash = data.get().find("hash");
                    if (login == data.get().cend() || hash == data.get().cend())
                    {
                        ec.get() = asio::error::invalid_argument;
                        std::cout << "bad" << std::endl;
                    }
                    else
                    {
                        auto isEq = sharedContext->GetDbSession().CompareSendedAuthDataWithDb(std::make_pair(login->get<std::string>(), hash->get<std::string>()));
                        Json answer;
                        if(isEq){
                            answer["AuthResult"] = "Passed";
                        }
                        else{
                            answer["AuthResult"] = "Not Passed";
                        }
                        jsonSession.get().SendJson(answer, ec.get(), yield);
                    };
                }
                catch (std::exception &ex)
                {
                    ec.get() = asio::error::invalid_argument;
                }
            }
        };
        result.insert(std::make_pair("GetAuthResult", std::move(CompareHashs)));
    }
    return result;
}
std::optional<CallableReaction> ServerDemon::GetReaction(const std::string &requestType)
{
    static auto ReactionsCont = ConstructReacions();
    auto it = ReactionsCont.find(requestType);
    std::optional<CallableReaction> result;
    if(it != ReactionsCont.cend()){
        result = (*it).second;
    }
    return result;
}
void ServerDemon::do_clientSession(JsonMq &jsonMqSession, boost::system::error_code &ec)
{
    auto self = shared_from_this();
    auto dataForSpawnFactory = [self, &ec, &jsonMqSession]()
    {
        return [self, &ec, &jsonMqSession](asio::yield_context yield) mutable
        {
            while (true)
            {
                auto data = jsonMqSession.GetJson(ec, yield);
                try{
                    if(ec){
                        throw std::exception();
                    }
                    auto typeIt = data.find("Type");
                    if(typeIt == data.cend()){
                        throw std::exception();
                    }
                    else{
                        auto reaction = self->GetReaction(typeIt->get<std::string>());
                        if(reaction.has_value()){
                            reaction.value()(std::ref(jsonMqSession), self, std::ref(data), std::ref(ec), yield);
                        }
                        else{
                            throw std::exception();
                        }
                    }
                } catch (std::exception& ex){
                    ec = asio::error::invalid_argument;
                }
            }
        };
    };
    asio::spawn(self->sysService, dataForSpawnFactory());
}
void ServerDemon::do_accept(JsonMq &jsonMqSession, boost::system::error_code &ec, asio::yield_context yield)
{
    auto self = shared_from_this();
    jsonMqSession = JsonMq(self->sysService, self->acc, ec, yield);
    if (ec.value() == 0)
    {
        std::cout << "Connection Ok" << std::endl;
    }
    else
    {
        std::cout << "Bad conn " << ec.message() << std::endl;
    }
}

ServerDemon::ServerDemon() : ep(asio::ip::tcp::v4(), 2001), acc(sysService, ep) {}
void ServerDemon::RunThread(asio::yield_context yield)
{
    try
    {
        auto self = shared_from_this();
        while (true)
        {
            JsonMq jsonMqSession;
            boost::system::error_code code;
            self->do_accept(jsonMqSession, code, yield);
            if (code.value() == 0)
            {
                do_clientSession(jsonMqSession, code);
            }
            boost::this_thread::interruption_point();
        }
    }
    catch (boost::thread_interrupted &inter)
    {
        return;
    }
}
std::shared_ptr<ServerDemon> ServerDemon::GetPtr()
{
    return shared_from_this();
}

std::shared_ptr<ServerDemon> ServerDemon::Create()
{
    return std::shared_ptr<ServerDemon>(new ServerDemon(), ServerDeleter<ServerDemon>());
}
void ServerDemon::RunDemon()
{
    auto self = shared_from_this();
    std::cout << "Get self" << std::endl;
    auto runYield = [self](asio::yield_context yield)
    {
        self->RunThread(yield);
    };
    auto run = [&runYield, self]()
    {
        asio::spawn(self->sysService, runYield);
        self->sysService.run();
    };
    for (int i = 0; i < 1; i++)
    {
        self->threads.create_thread(run);
    }
    std::string answer;
    std::cout << "Server started" << std::endl;
    while (true)
    {
        std::cin >> answer;
        if (answer == "exit")
        {
            std::cout << "Bye" << std::endl;
            break;
        }
    }
}
