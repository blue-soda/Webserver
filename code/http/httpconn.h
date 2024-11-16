#ifndef HTTP_HPP
#define HTTP_HPP
#include <boost/beast/http.hpp>
#include <boost/beast/core.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/write.hpp>
#include <boost/json.hpp>
#include <boost/filesystem.hpp>
#include <boost/locale.hpp>
#include <iomanip>
#include <memory>
#include <string>
#include <map>
#include "../log/log.h"
#include "../http/sqlhandler.hpp"


namespace beast = boost::beast;
namespace http = beast::http; 
namespace asio = boost::asio;
namespace json = boost::json;
namespace filesystem = boost::filesystem;
using tcp = asio::ip::tcp;                  // TCP


const std::string DATA_DIR = "../static/data/";
const std::string INDEX_HTML_PATH = "../static/index.html";
const std::string AVATAR_DIR = "../static/avatar/";


std::string getQueryParameter(const std::string& url, const std::string& param);
std::string urlDecode(const std::string& str);
std::string urlEncode(const std::string &value);


class HttpConn {
public:
    HttpConn(asio::io_service& io_service, std::shared_ptr<tcp::socket> socket, std::shared_ptr<SqlHandler> sqlhandler) : 
        io_service_(io_service), socket_(socket), sqlHandler_(sqlhandler)  {}
    static std::multimap<long long int, json::object> chatHistory_;
    void generateResponse(const std::string& body, const std::string& content_type = "text/html", 
        http::status status = http::status::ok);
    void handleClient();
    void handleRequest();
    void close();
    void sendFile(const std::string& file_path);
private:
    static std::mutex mtx_;
    std::shared_ptr<SqlHandler> sqlHandler_;
    asio::io_service& io_service_;
    std::shared_ptr<tcp::socket> socket_;
    beast::flat_buffer buffer_;
    http::request_parser<http::string_body> parser_;
    http::request<http::string_body> request_;
    Logger logger_;
};

#endif