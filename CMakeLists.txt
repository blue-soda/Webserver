cmake_minimum_required(VERSION 3.10)

project(WebServer)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_BUILD_TYPE Debug)

set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_C_FLAGS_DEBUG "-g")

set(BOOST_DIR /usr/include/boost)
set(MYSQL_INCLUDE_DIR /usr/include)
set(MYSQL_LIB_DIR /usr/lib/x86_64-linux-gnu)

find_package(Boost REQUIRED COMPONENTS filesystem system json)

file(GLOB_RECURSE SRC_FILES "code/**/*.cpp" "code/*.cpp")

include_directories(
    ${MYSQL_INCLUDE_DIR}
    ${BOOST_DIR}
)

link_directories(${MYSQL_LIB_DIR})

add_executable(webserver ${SRC_FILES})

target_link_libraries(webserver
    mysqlclient
    pthread
    Boost::filesystem
    Boost::system
    Boost::json
    mysqlcppconn
)


set_target_properties(webserver PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)
