#ifndef WEBSERVER_HPP
#define WEBSERVER_HPP
#include <iostream>
#include <boost/asio.hpp>
#include <fstream>
#include <string>
#include <memory>
#include <vector>
#include <unordered_map>
#include "../pool/sqlconnpool.hpp"
#include "../pool/threadpool.hpp"
#include "../http/httpconn.h"
#include "../http/sqlhandler.hpp"
#include "../log/log.h"

class WebServer{
public:
    WebServer(const std::string& serverIP, const short& serverPort,
        const std::string& sqlUrl, const std::string& sqlUser, const std::string& sqlPwd,
        const std::string& dbName, int sqlPoolNum, int threadNum);

    ~WebServer();
    void start();
    void stop(int signum = 0);
    
private:
    std::string SERVER_IP;
    short SERVER_PORT;

    std::shared_ptr<SqlConnPool> sqlConnPool_;
    std::shared_ptr<SqlHandler> sqlHandler_;
    std::unique_ptr<ThreadPool> threadPool_;

    asio::io_context io_context_;
    tcp::acceptor acceptor_;

    void startAccept();

    Logger logger_;


};
#endif