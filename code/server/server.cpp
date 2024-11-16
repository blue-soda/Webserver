#include "server.h"

WebServer::WebServer(const std::string& serverIP, const short& serverPort,  
        const std::string& sqlUrl, const std::string& sqlUser, const std::string& sqlPwd,
        const std::string& dbName, int sqlPoolNum, int threadNum) : 
        SERVER_IP(serverIP), SERVER_PORT(serverPort), 
        acceptor_(tcp::acceptor(io_context_, tcp::endpoint(asio::ip::make_address(SERVER_IP), SERVER_PORT))){
    logger_ << "WebServer initializing...";
    asio::socket_base::reuse_address option(true);
    acceptor_.set_option(option);
    logger_ << "SqlConnPool num: " + std::to_string(sqlPoolNum) + ", ThreadPool num: " + std::to_string(threadNum);
    sqlConnPool_ = std::make_shared<SqlConnPool>(sqlUrl, sqlUser, sqlPwd, dbName, sqlPoolNum);
    sqlHandler_ = std::make_shared<SqlHandler>(sqlConnPool_);
    threadPool_ = std::make_unique<ThreadPool>(threadNum);
    logger_ << "WebServer initialized.";
}

WebServer::~WebServer() {
    logger_ << "Shutting down WebServer...";
    io_context_.stop();
    logger_ << "IO context stopped.";
}

void WebServer::start(){
    startAccept();
    io_context_.run();
}

void WebServer::stop(int signum){
    logger_ << "Shutting down WebServer on Signal " + std::to_string(signum);
    io_context_.stop();
    logger_ << "IO context stopped.";    
}

void WebServer::startAccept(){
    logger_ << "WebServer: startAccept.";

    auto socket = std::make_shared<tcp::socket>(io_context_);

    acceptor_.async_accept(*socket, [this, socket](const boost::system::error_code& ec) {
        if (!ec) {
            logger_ << "WebServer: new httpconn.";
            auto httpconn = std::make_shared<HttpConn>(io_context_, socket, sqlHandler_);
            threadPool_->submit([=]() { httpconn->handleClient(); });
        } else {
            logger_.error();
            logger_ << "WebServer: asyn_accept Error: " << ec.message();
            logger_.info();
        }

        startAccept();
    });
}




