cmake_minimum_required(VERSION 3.5)
set(PROJECT_NAME_MY server)
project(${PROJECT_NAME_MY} LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(JSON_BuildTests OFF CACHE INTERNAL "")

include_directories(${CMAKE_SOURCE_DIR}/third_party/libJson1/include)
include_directories(${CMAKE_SOURCE_DIR}/third_party/libJson1)
include_directories(${CMAKE_SOURCE_DIR}/third_party/codec/cppcodec)
add_subdirectory(${CMAKE_SOURCE_DIR}/third_party/libJson1)
add_subdirectory(${CMAKE_SOURCE_DIR}/third_party/codec)

#target_link_libraries(mysqlpp INTERFACE mysqlinterface)
FIND_PACKAGE( Boost 1.82 REQUIRED COMPONENTS thread system coroutine)
#set(OPENSSL_USE_STATIC_LIBS TRUE)
find_package(OpenSSL REQUIRED)
INCLUDE_DIRECTORIES( ${Boost_INCLUDE_DIR} )
link_libraries(${OPENSSL_LIBRARIES})

add_library(${PROJECT_NAME_MY}_Rc4 Rc4.hpp Rc4.cpp)

add_library(${PROJECT_NAME_MY}_MyWebSocket MyWebSocket.h MyWebSocket.cpp)
target_link_libraries( ${PROJECT_NAME_MY}_MyWebSocket ${Boost_LIBRARIES} OpenSSL::Crypto ${PROJECT_NAME_MY}_Rc4)


add_library(${PROJECT_NAME_MY}_JsonMaster JsonMaster.h JsonMaster.cpp)
add_library(${PROJECT_NAME_MY}_DBMaster DBMaster.h DBMaster.cpp)
target_link_libraries( ${PROJECT_NAME_MY}_DBMaster PUBLIC ${Boost_LIBRARIES} OpenSSL::Crypto ${PROJECT_NAME_MY}_JsonMaster)
add_library(${PROJECT_NAME_MY}_JsonMq JsonMq.h JsonMq.cpp)
target_link_libraries( ${PROJECT_NAME_MY}_JsonMq ${Boost_LIBRARIES} ${PROJECT_NAME_MY}_MyWebSocket)

add_library(${PROJECT_NAME_MY}_helman DiffiHelman.h DiffiHelman.cpp)
target_link_libraries(${PROJECT_NAME_MY}_helman ${Boost_LIBRARIES})

add_library(${PROJECT_NAME_MY}_demon Server.h Server.cpp)
target_link_libraries(${PROJECT_NAME_MY}_demon OpenSSL::Crypto ${PROJECT_NAME_MY}_Rc4)
target_link_libraries( ${PROJECT_NAME_MY}_demon ${Boost_LIBRARIES} ${PROJECT_NAME_MY}_JsonMq OpenSSL::Crypto)
target_link_libraries(${PROJECT_NAME_MY}_demon  ${PROJECT_NAME_MY}_DBMaster ${PROJECT_NAME_MY}_JsonMaster ${PROJECT_NAME_MY}_helman)
add_executable(${PROJECT_NAME_MY}_ex main.cpp)
target_link_libraries(${PROJECT_NAME_MY}_ex ${PROJECT_NAME_MY}_demon ${Boost_LIBRARIES} OpenSSL::Crypto)
