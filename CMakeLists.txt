# CMake 最低版本要求
cmake_minimum_required(VERSION 3.10)

# 项目名称
project(WebServer)

# 使用 C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置构建类型为 Debug
set(CMAKE_BUILD_TYPE Debug)

# 选择调试标志
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_C_FLAGS_DEBUG "-g")

# 设置 Boost 和 MySQL Connector C++ 库的路径
set(BOOST_DIR /usr/include/boost)
set(MYSQL_INCLUDE_DIR /usr/include)
set(MYSQL_LIB_DIR /usr/lib/x86_64-linux-gnu)

# 查找 Boost 库
find_package(Boost REQUIRED COMPONENTS filesystem system json)

# 查找项目源文件
file(GLOB_RECURSE SRC_FILES "code/**/*.cpp" "code/*.cpp")

# 添加头文件目录
include_directories(
    ${MYSQL_INCLUDE_DIR}
    ${BOOST_DIR}
)

# MySQL C++ Connector 库路径
link_directories(${MYSQL_LIB_DIR})

# 创建目标可执行文件
add_executable(webserver ${SRC_FILES})

# 链接 MySQL 和 Boost 库
target_link_libraries(webserver
    mysqlclient
    pthread
    Boost::filesystem
    Boost::system
    Boost::json
    mysqlcppconn
)

# 如果需要，可以设置一些额外的构建配置
# 例如，设置输出路径等
set_target_properties(webserver PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# 可选：在生成构建文件之前进行清理
# add_custom_target(clean-all COMMAND rm -rf ${CMAKE_BINARY_DIR}/*)
