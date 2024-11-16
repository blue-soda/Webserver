#ifndef SQL_POOL_HPP
#define SQL_POOL_HPP
//#include <mysql/mysql.h>
#include <cppconn/driver.h>
#include <cppconn/connection.h>
#include <mysql_connection.h>
#include <mysql_driver.h>
#include <cppconn/exception.h>
#include <cppconn/statement.h>
#include <cppconn/resultset.h>
#include <cppconn/prepared_statement.h>
#include <string>
#include <queue>
#include <memory>
#include "../log/log.h"
#include <condition_variable>

class SqlConnPool{
public:
    SqlConnPool(const std::string& sqlUrl, const std::string& sqlUser, const std::string& sqlPwd,
        const std::string& dbName, int sqlPoolNum) : 
        url_(sqlUrl), user_(sqlUser), password_(sqlPwd), database_(dbName), poolSize_(sqlPoolNum){
        logger_.info();
        logger_ << "MYSQL pool initializing...";

        //driver_ = std::shared_ptr<sql::Driver>(get_driver_instance());
        driver_ = get_driver_instance();
        logger_ << "MYSQL Driver initialized";
        initDatabase();
        logger_ << "MYSQL Database initialized";

        for(int i = 0; i < sqlPoolNum; i++){
            try{
                connPool_.push(std::move(createConnection()));
            }
            catch(const std::exception& e){
                logger_.error();
                logger_ << "SqlConnPool: CreateSqlConn Failed: " << e.what();
            }
            // MYSQL * conn = mysql_init(nullptr);
            // if (conn == nullptr) {
            //     logger_.error();
            //     logger_ << "mysql_init failed!";
            //     continue;
            // }
            // conn = mysql_real_connect(conn, sqlHost.c_str(), sqlUser.c_str(), sqlPwd.c_str(),
            //                 dbName.c_str(), sqlPort, nullptr, 0);
        
            // if (conn) {
            //     std::unique_lock<std::mutex> lock(mtx_);
            //     connPool_.push(conn);
            // } else {
            //     logger_.error();
            //     logger_ << "mysql_real_connect failed!!";
            // }
        }
        logger_.info();
        logger_ << "MYSQL pool initialized.";
    }

    ~SqlConnPool(){
        logger_.info();
        logger_ << "MYSQL pool closing.";
        std::lock_guard<std::mutex> lock(mtx_);
        while(not connPool_.empty()){
            connPool_.front()->close();
            connPool_.pop();
        }
        logger_ << "MYSQL pool closed.";
    }
    //MYSQL * getSqlConn(){
    std::shared_ptr<sql::Connection> getSqlConn(){
        std::unique_lock<std::mutex> lock(mtx_);
        while(connPool_.empty()){
            condVar_.wait(lock);
        }
        //MYSQL * conn = connPool_.front();
        auto conn = connPool_.front();
        connPool_.pop();
        return conn;
    }

    //void releaseSqlConn(MYSQL * conn){
    void releaseSqlConn(std::shared_ptr<sql::Connection> conn){
        std::unique_lock<std::mutex> lock(mtx_);
        connPool_.push(conn);
        condVar_.notify_one();
    }

    void initDatabase(){
        try{
            std::shared_ptr<sql::Connection> conn(driver_->connect(url_, user_, password_));
        }
        catch(const sql::SQLException& e){
            logger_.error();
            logger_ << "SqlConnPool: Connecting to " + url_ + " Failed:" << e.what();
            return;
        } 
        std::shared_ptr<sql::Connection> conn(driver_->connect(url_, user_, password_));
        std::shared_ptr<sql::Statement> stmt(conn->createStatement());
        std::shared_ptr<sql::ResultSet> res(stmt->executeQuery(
            "SHOW DATABASES LIKE '" + database_ + "'"
        ));
        if(res->next()){
            logger_.info();
            logger_ << "SqlConnPool: " + database_ + " connected.";
        }
        else{
            logger_.info();
            logger_ << "SqlConnPool: " + database_ + " does not exist. Creating...";
            try{
                std::shared_ptr<sql::Connection> conn(driver_->connect(url_, user_, password_));
                std::shared_ptr<sql::Statement> stmt(conn->createStatement());
                stmt->execute("CREATE DATABASE " + database_);
                logger_.info();
                logger_ << "SqlConnPool: " + database_ + " created.";
            }
            catch(const sql::SQLException& e){
                logger_.error();
                logger_ << "SqlConnPool: Create Database Failed: " << e.what();
            }
        }
    }

private:
    std::shared_ptr<sql::Connection> createConnection(){
        std::shared_ptr<sql::Connection> conn(driver_->connect(url_, user_, password_));
        std::unique_ptr<sql::Statement> stmt(conn->createStatement());
        stmt->execute("USE " + database_);  
        return conn;
    }
    sql::Driver* driver_;
    //std::shared_ptr<sql::Driver> driver_;
    //std::queue<MYSQL *> connPool_;
    std::queue<std::shared_ptr<sql::Connection>> connPool_;
    std::mutex mtx_;
    Logger logger_;
    std::condition_variable condVar_;

    std::string url_;
    std::string user_;
    std::string password_;
    std::string database_;
    int poolSize_;
};
#endif