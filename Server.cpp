#include "Server.h"
#include <algorithm>
#include <sstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <fstream>
#include <codecvt>
#include <locale>
#include <boost/beast.hpp>
#include <openssl/sha.h>
#include <string>
#include <iostream>
#include <cstdint>
#include <tuple>
#include "JsonMaster.h"
#include <boost/coroutine/coroutine.hpp>
using namespace std::string_literals;
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
                try
                {
                    auto registrData = FromJson<RegistrData>(data.get());
                    if (registrData.has_value())
                    {
                        std::cout << "Sending to db from reaction" << std::endl;
                        sharedContext->GetDbSession().SendRegistrDataToDB(registrData.value());
                    }
                    else{
                        throw std::exception();
                    }
                }
                catch (std::exception &ex)
                {
                    ec.get() = asio::error::invalid_argument;
                }
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
                try
                {
                    auto wordRequest = FromJson<KeyWordRequest>(data.get());
                    if (wordRequest.has_value())
                    {
                        auto keyWord = sharedContext->GetDbSession().GetWordFromDb(wordRequest.value());
                        KeyWord answer{keyWord : keyWord};
                        Json jAnswer = ToJson(answer);
                        jsonSession.get().SendJson(jAnswer, ec, yield);
                    }
                    else{
                        throw std::exception();
                    }
                }
                catch (std::exception &e)
                {
                    ec.get() = asio::error::invalid_argument;
                }
            }
        };
        result.insert(std::make_pair("GetKeyWordRequest", std::move(SendWordToClient)));
    }
    {
        auto CompareHashs = [](MyRf<JsonMq> jsonSession, std::weak_ptr<ServerDemon> serverSession, MyRf<Json> data, MyRf<boost::system::error_code> ec, asio::yield_context yield)
        {
            auto sharedContext = serverSession.lock();
            if (sharedContext != nullptr)
            {
                try
                {
                    auto authData = FromJson<AuthData>(data.get());
                    if(authData.has_value()){
                        bool compareResult = sharedContext->GetDbSession().CompareSendedAuthDataWithDb(authData.value());
                        AuthDataAnswer answer;
                        if(compareResult){
                            answer.passed = "Passed";
                        }
                        else{
                            answer.passed = "Not Passed";
                        }
                        auto jAnswer = ToJson(answer);
                        jsonSession.get().SendJson(jAnswer, ec.get(), yield);
                    }
                    else{
                        throw std::exception();
                    }
                   
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
    if (it != ReactionsCont.cend())
    {
        result = (*it).second;
    }
    return result;
}
void ServerDemon::do_clientSession(JsonMq &&jsonMqSession)
{
    auto self = shared_from_this();
    auto dataForSpawnFactory = [self, jsonMqSession = std::move(jsonMqSession)]()
    {
        return [self, jsonMqSession = std::move(jsonMqSession)](asio::yield_context yield) mutable
        {
            boost::system::error_code ec;
            while (true)
            {
                try
                {
                    std::cout << "Start of getting reaction" << std::endl;
                    auto data = jsonMqSession.GetJson(ec, yield);
                    if (ec)
                    {
                        throw std::exception();
                    }
                    auto typeIt = data.find("Type");
                    if (typeIt == data.cend())
                    {
                        throw std::exception();
                    }
                    else
                    {
                        std::cout << "Reaction type " << *typeIt << std::endl;
                        auto reaction = self->GetReaction(typeIt->get<std::string>());
                        if (reaction.has_value())
                        {
                            reaction.value()(std::ref(jsonMqSession), self, std::ref(data), std::ref(ec), yield);
                            std::cout << "Reaction handled" << std::endl;
                        }
                        else
                        {
                            throw std::exception();
                        }
                    }
                }
                catch (std::exception &ex)
                {
                    ec = asio::error::invalid_argument;
                    std::cout << "exceiption handled in reaction" << std::endl;
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
                do_clientSession(std::move(jsonMqSession));
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
