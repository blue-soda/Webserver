#include "./server/server.h"
#include "./log/log.h"

int main(){
    Logger logger;
    logger << std::endl << std::endl << std::endl;
    logger << "==================== Server init ====================";
    WebServer server("10.147.17.2", 8080, 
        "tcp://localhost:3306", "sakakibara", "password",
        "Users", 10, 10);
    logger << "==================== Server start ====================";
    server.start();
    return 0;
}
