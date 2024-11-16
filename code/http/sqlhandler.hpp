#ifndef SQLHANDLER_HPP
#define SQLHANDLER_HPP
#include "../pool/sqlconnpool.hpp"
class SqlHandler{
public:
    SqlHandler(std::shared_ptr<SqlConnPool> pool, const std::string& tableName = "user_info") :
        connPool_(pool), tableName_(tableName){
        std::string createTableQuery = "CREATE TABLE IF NOT EXISTS " + tableName_ + R"( 
            (
            id INT AUTO_INCREMENT PRIMARY KEY,
            username VARCHAR(50) NOT NULL UNIQUE,
            password VARCHAR(50) NOT NULL,
            contact VARCHAR(100),
            avatar VARCHAR(100)
            )
        )";
        auto conn = connPool_->getSqlConn();
        try{
            std::shared_ptr<sql::Statement> stmt(conn->createStatement());
            stmt->executeUpdate(createTableQuery);
            logger_.info() << "SqlHandler: Table Created.";
        }
        catch(const sql::SQLException& e){
            logger_.error() << "SqlHandler: Table Create Failed:" << e.what();
        }
        connPool_->releaseSqlConn(conn);
    }

    bool insertUser(const std::string& username, const std::string& password, 
        const std::string& contact_info = ""){
        auto conn = connPool_->getSqlConn();
        bool ret = false;
        try{
            std::unique_ptr<sql::PreparedStatement> pstmt(
                conn->prepareStatement("INSERT INTO " + tableName_ + " (username, password, contact) VALUES (?, ?, ?)"));
            pstmt->setString(1, username);
            pstmt->setString(2, password);
            pstmt->setString(3, contact_info);
            pstmt->executeUpdate();
            logger_.info() << "SqlHandler: InsertUser Successed.";
            ret = true;
        }
        catch(const sql::SQLException& e){
            logger_.error() << "SqlHandler: InsertUser Failed: " << e.what();
        }
        connPool_->releaseSqlConn(conn);
        return ret;
    }

    bool deleteUser(const std::string& username){
        auto conn = connPool_->getSqlConn();
        bool ret = false;
        try{
            std::unique_ptr<sql::PreparedStatement> pstmt(
                conn->prepareStatement("DELETE FROM " + tableName_ + " WHERE username = ?"));
            pstmt->setString(1, username);
            pstmt->executeUpdate();
            logger_.info() << "SqlHandler: deleteUser Successed.";
            ret = true;
        }
        catch(const sql::SQLException& e){
            logger_.error() << "SqlHandler: deleteUser Failed: " << e.what();
        }
        connPool_->releaseSqlConn(conn);
        return ret;
    }

    std::shared_ptr<sql::ResultSet> queryUser(const std::string& username){
        auto conn = connPool_->getSqlConn();
        std::shared_ptr<sql::ResultSet> ret(nullptr);
        try{
            std::unique_ptr<sql::PreparedStatement> pstmt(
                conn->prepareStatement("SELECT * FROM " + tableName_ + " WHERE username = ?"));
            pstmt->setString(1, username);
            std::shared_ptr<sql::ResultSet> res(pstmt->executeQuery());
            logger_.info() << "SqlHandler: queryUser Successed.";
            ret = res;
        }
        catch(const sql::SQLException& e){
            logger_.error() << "SqlHandler: queryUser Failed: " << e.what();
        }
        connPool_->releaseSqlConn(conn);
        return ret;
    }

    bool authenticateUser(const std::string& username, const std::string& password){
        bool ret = false;
        auto conn = connPool_->getSqlConn();
        try{
            std::unique_ptr<sql::PreparedStatement> pstmt(
                conn->prepareStatement("SELECT * FROM " + tableName_ + " WHERE username = ? AND password = ?"));
            pstmt->setString(1, username);
            pstmt->setString(2, password);
            std::shared_ptr<sql::ResultSet> res(pstmt->executeQuery());
            logger_.info() << "SqlHandler: authenticateUser Successed.";
            if(res->next())
                ret = true;
        }
        catch(const sql::SQLException& e){
            logger_.error() << "SqlHandler: authenticateUser Failed: " << e.what();
        }
        connPool_->releaseSqlConn(conn);
        return ret;
    }

    bool updateTable(const std::string& username, const std::string key, const std::string value){
        auto conn = connPool_->getSqlConn();
        bool ret = false;
        try{
            std::unique_ptr<sql::PreparedStatement> pstmt(
                conn->prepareStatement("UPDATE " + tableName_ + " SET " + key + " = ? WHERE username = ?"));
            pstmt->setString(1, value);
            pstmt->setString(2, username);
            pstmt->executeUpdate();
            logger_.info() << "SqlHandler: updateUser " + key + " Successed: " + value;
            ret = true;
        }
        catch(const sql::SQLException& e){
            logger_.error() << "SqlHandler: updateUser " + key + " Failed: " << e.what();
            ret = false;
        }
        connPool_->releaseSqlConn(conn);
        return ret;
    }
    
    bool updateUser(const std::string& username, 
        const std::string& password = "", const std::string& contact = "", const std::string& newName = ""){
        std::string key = "", value = "";
        bool ret = true;
        if(!password.empty()){
            key = "password";
            value = password;
            ret = ret && updateTable(username, key, value);
        }
        if(!contact.empty()){
            key = "contact";
            value = contact;
            ret = ret && updateTable(username, key, value);
        }
        if(!newName.empty()){
            key = "username";
            value = newName;
            ret = ret && updateTable(username, key, value);
        }
        return ret;
    }

    
private:
    std::shared_ptr<SqlConnPool> connPool_;
    std::string tableName_ = "user_info";
    Logger logger_;
};

#endif