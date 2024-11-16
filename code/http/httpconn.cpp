#include "httpconn.h"

std::multimap<long long int, boost::json::object> HttpConn::chatHistory_;
std::mutex HttpConn::mtx_;

//List all the files under the directory
std::string listFilesAsJson(const std::string& directory) {
    json::array fileList;

    for (const auto& entry : filesystem::directory_iterator(directory)) {
        if (filesystem::is_regular_file(entry)){ //&& entry.path().extension() == ".pdf") {
            fileList.emplace_back(entry.path().filename().string());
        }
    }

    json::object result;
    result["files"] = fileList;
    return json::serialize(result);
}


void HttpConn::generateResponse(const std::string& body, const std::string& content_type, http::status status){
    http::response<http::string_body> res{status, 11}; //status 200
    res.set(http::field::content_type, content_type);
    res.set(http::field::content_length, std::to_string(body.size()));
    res.set(http::field::server, "Boost.Beast HTTP Server");
    res.body() = body;
    // http::async_write(socket_, res, [&](const boost::system::error_code& ec, std::size_t bytes_transferred) {
    //     if (ec) {
    //         logger_.Log(std::string("Error sending response: ") + ec.message(), LogLevel::ERROR);
    //     }});
    http::write(*socket_, res);
}


void HttpConn::handleRequest(){
    std::string target = std::string(request_.target());
    if (target == "/file-list") {
        std::string jsonResponse = listFilesAsJson(DATA_DIR);
        generateResponse(jsonResponse, "application/json");
        logger_ << "HttpConn: file-list";
    } 
    else if(target.rfind("/download/", 0) == 0) { 
        std::string filename = target.substr(10); 
        sendFile(DATA_DIR + filename);
        logger_ << std::string("HttpConn: download file: ") + filename;
    }   
    else if (target == "/") {  
        std::ifstream file(INDEX_HTML_PATH);
        if (file.is_open()) {
            std::stringstream buffer;
            buffer << file.rdbuf();
            std::string body = buffer.str();
            generateResponse(body, "text/html", http::status::ok);
            logger_ << "HttpConn: Serving index.html";
        } else {
            std::string body = "<html><body><h1>404 - Not Found</h1></body></html>";
            generateResponse(body, "text/html", http::status::not_found);
            logger_ << "HttpConn: index.html not found, sent 404";
        }
    }   
    else if(target == "/login" && request_.method() == http::verb::post) { 

        std::string body = request_.body();

        json::value jv = json::parse(body);
        json::object requestData = jv.as_object();
        
        std::string username = json::value_to<std::string>(requestData["username"]);
        std::string password = json::value_to<std::string>(requestData["password"]);

        bool loginSuccess = sqlHandler_->authenticateUser(username, password);

        json::object responseJson;
        if (loginSuccess) {
            responseJson["success"] = true;
            responseJson["message"] = "Login successful";
        } else {
            responseJson["success"] = false;
            responseJson["message"] = "Invalid username or password";
        }

        generateResponse(json::serialize(responseJson), "application/json");

        logger_ << "HttpConn: login request";
    }
    else if(target == "/register" && request_.method() == http::verb::post) { 

        std::string body = request_.body();
        json::value jv = json::parse(body);
        json::object requestData = jv.as_object();
        
        std::string username = json::value_to<std::string>(requestData["username"]);
        std::string password = json::value_to<std::string>(requestData["password"]);
        std::string contact = json::value_to<std::string>(requestData["contact"]);

        bool registerSuccess = sqlHandler_->insertUser(username, password, contact);//authenticateUser(username, password);

        json::object responseJson;
        if (registerSuccess) {
            responseJson["success"] = true;
            responseJson["message"] = "Register Successed.";
        } else {
            responseJson["success"] = false;
            responseJson["message"] = "Register Failed.";
        }

        generateResponse(json::serialize(responseJson), "application/json");

        logger_ << "HttpConn: register request";
    }
    else if(target == "/update-profile" && request_.method() == http::verb::post) { 

        std::string body = request_.body();
        json::value jv = json::parse(body);
        json::object requestData = jv.as_object();
        
        std::string username = json::value_to<std::string>(requestData["username"]);
        std::string currentPassword = json::value_to<std::string>(requestData["currentPassword"]);
        std::string newPassword = json::value_to<std::string>(requestData["newPassword"]);
        std::string contact = json::value_to<std::string>(requestData["contact"]);
        std::string newName = json::value_to<std::string>(requestData["newName"]);

        bool authenticateSuccess = true;
        if(!newPassword.empty())
            authenticateSuccess = sqlHandler_->authenticateUser(username, currentPassword);

        json::object responseJson;
        if (authenticateSuccess) {
            bool updateSuccess = sqlHandler_->updateUser(username, newPassword, contact, newName);
            if(updateSuccess){
                responseJson["success"] = true;
                responseJson["message"] = "Update Successed.";
            }
        } else {
            responseJson["success"] = false;
            responseJson["message"] = "Update Failed.";
        }

        generateResponse(json::serialize(responseJson), "application/json");

        logger_ << "HttpConn: Update request";
    }
    else if(target.rfind("/query-user") == 0){
        std::string username = getQueryParameter(std::string(request_.target()), "username");
        json::object responseJson;
        try{
            std::shared_ptr<sql::ResultSet> res = sqlHandler_->queryUser(username);
            res->next();
            std::string contact = res->getString("contact");
            std::string avatar = res->getString("avatar");
            if(avatar.empty()){
                avatar = "default-avatar.jpg";
            }
            responseJson["success"] = true;
            responseJson["contact"] = contact;
            responseJson["avatar"] = avatar;
            logger_.info() << "HttpConn: QueryUser Successed: " << "Contact: " + contact << "Avatar: " + avatar;
        }
        catch(const sql::SQLException& e){
            logger_.error() << "HttpConn: QueryUser Faild: " << e.what();
            responseJson["success"] = false;
        }
        generateResponse(json::serialize(responseJson), "application/json");
    }
    else if(target == "/send-message" && request_.method() == http::verb::post){
        std::string body = request_.body();
        json::value jv = json::parse(body);
        json::object requestData = jv.as_object();

        std::string username = json::value_to<std::string>(requestData["senderId"]);
        std::string message = json::value_to<std::string>(requestData["text"]);
        long long int timestamp = json::value_to<long long int>(requestData["timestamp"]);

        json::object responseJson;
        bool sendMessageSuccess = true;
        if (sendMessageSuccess){
            std::lock_guard<std::mutex> lock(mtx_);
            responseJson["success"] = true;
            chatHistory_.insert(std::make_pair(timestamp, std::move(requestData)));
            logger_ << "HttpConn: ChatHistory Added, Now Have " + std::to_string(chatHistory_.size()) + " Messages.";
        }
        else
            responseJson["success"] = false;
        generateResponse(json::serialize(responseJson), "application/json");
        logger_ << "HttpConn: Get Message At " + std::to_string(timestamp) + " From User " + username + ": " + message;
    }
    else if(target.rfind("/get-chat-messages") == 0){
        json::object responseJson;
        std::string afterStr = getQueryParameter(std::string(request_.target()), "after");
        if (afterStr.empty()) {
                responseJson["text"] = "Missing 'after' query parameter";
                responseJson["success"] = false;
                generateResponse(json::serialize(responseJson), "application/json", http::status::bad_request);
            logger_ << "HttpConn: bad_request.";
            return;
        }
        json::array messages;

        long long int currentTimeStamp = std::stoll(afterStr.c_str());
        logger_ << "HttpConn: Request Chat Message After: " + std::to_string(currentTimeStamp);
        auto itUpper = chatHistory_.upper_bound(currentTimeStamp);
        for (auto it = itUpper; it != chatHistory_.end(); ++it) {
            json::object obj = it->second;
            std::string username = json::value_to<std::string>(obj["senderId"]);
            std::string message = json::value_to<std::string>(obj["text"]);
            long long int timestamp = json::value_to<long long int>(obj["timestamp"]);
            std::string avatarUrl = "default-avatar.jpg";
            try{
                std::shared_ptr<sql::ResultSet> res = sqlHandler_->queryUser(username);
                res->next();
                avatarUrl = res->getString("avatar");
            }
            catch(const sql::SQLException& e){
                avatarUrl = "default-avatar.jpg";
                logger_.error() << "HttpConn: Query Avatar Failed While Getting Chat Messages: " << e.what();
            }
            messages.push_back({
                {"timestamp", timestamp},
                {"senderId", username},
                {"text", message},
                {"avatarUrl", avatarUrl}
            });
        }

        responseJson["messages"] = messages;
        generateResponse(json::serialize(responseJson), "application/json");
        logger_ << "HttpConn: Chat Messages Sent.";
    }
    else if(target.rfind("/avatar") == 0){
        std::string file_path = AVATAR_DIR + target.substr(8);
        sendFile(file_path);
        logger_ << "HttpConn: Sent Avatar " + file_path;
    }
    else if (target.rfind("/upload-avatar") == 0 && request_.method() == http::verb::post) {
        json::object responseJson;
        std::string username = getQueryParameter(std::string(request_.target()), "username");
        logger_.info() << "HttpConn: " + username + " Uploading Avatar.";
        try {
            responseJson["success"] = false;
            // 确保 Content-Type 是 multipart/form-data
            auto content_type = std::string(request_.at(http::field::content_type));
            if (content_type.find("multipart/form-data") == std::string::npos) {
                responseJson["message"] = "Invalid Content-Type. Expected multipart/form-data.";
                generateResponse(json::serialize(responseJson), "application/json", http::status::bad_request);
                return;
            }

            // find boundary_pos
            auto boundary_pos = content_type.find("boundary=");
            if (boundary_pos == std::string::npos) {
                responseJson["message"] = "Boundary not found in Content-Type.";
                generateResponse(json::serialize(responseJson), "application/json", http::status::bad_request);
                return;
            }
            std::string boundary = "--" + content_type.substr(boundary_pos + 9);

            // find body of file
            auto body = request_.body();
            auto start_pos = body.find(boundary);
            if (start_pos == std::string::npos) {
                responseJson["message"] = "Invalid multipart body.";
                generateResponse(json::serialize(responseJson), "application/json", http::status::bad_request);
                return;
            }

            // extract Content-Disposition
            start_pos = body.find("Content-Disposition:", start_pos);
            if (start_pos == std::string::npos) {
                responseJson["message"] = "Content-Disposition header not found.";
                generateResponse(json::serialize(responseJson), "application/json", http::status::bad_request);
                return;
            }

            // get filename
            auto filename_pos = body.find("filename=\"", start_pos);
            if (filename_pos == std::string::npos) {
                responseJson["message"] = "Filename not found in Content-Disposition.";
                generateResponse(json::serialize(responseJson), "application/json", http::status::bad_request);
                return;
            }
            filename_pos += 10; // skip `filename="`
            auto filename_end_pos = body.find("\"", filename_pos);
            if (filename_end_pos == std::string::npos) {
                responseJson["message"] = "Invalid filename in Content-Disposition.";
                generateResponse(json::serialize(responseJson), "application/json", http::status::bad_request);
                return;
            }
            std::string original_filename = body.substr(filename_pos, filename_end_pos - filename_pos);

            // get extension_name
            auto ext_pos = original_filename.find_last_of('.');
            std::string extension;
            if (ext_pos != std::string::npos) {
                extension = original_filename.substr(ext_pos);
            } else {
                responseJson["message"] = "File extension not found in filename.";
                generateResponse(json::serialize(responseJson), "application/json", http::status::bad_request);
                return;
            }

            // check extension
            std::set<std::string> supported_extensions = {".jpg", ".jpeg", ".png", ".gif"};
            if (supported_extensions.find(extension) == supported_extensions.end()) {
                responseJson["message"] = "Unsupported file type: " + extension;
                generateResponse(json::serialize(responseJson), "application/json", http::status::bad_request);
                return;
            }

            // locate file data
            start_pos = body.find("\r\n\r\n", filename_end_pos);
            if (start_pos == std::string::npos) {
                responseJson["message"] = "Invalid multipart body.";
                generateResponse(json::serialize(responseJson), "application/json", http::status::bad_request);
                return;
            }
            start_pos += 4; // skip `\r\n\r\n`

            auto end_pos = body.find(boundary, start_pos);
            if (end_pos == std::string::npos) {
                responseJson["message"] = "Invalid multipart body.";
                generateResponse(json::serialize(responseJson), "application/json", http::status::bad_request);
                return;
            }
            end_pos -= 2; // skip `\r\n`

            // extract content of file
            std::string file_data = body.substr(start_pos, end_pos - start_pos);

            // save the file under AVATAR_DIR
            std::string avatar = std::to_string(std::time(nullptr)) + extension;
            std::string filename = AVATAR_DIR + avatar;
            std::ofstream output(filename, std::ios::binary);
            if (!output.is_open()) {
                responseJson["message"] = "Failed to save the uploaded file.";
                generateResponse(json::serialize(responseJson), "application/json", http::status::internal_server_error);
                return;
            }
            output.write(file_data.data(), file_data.size());
            output.close();

            try{
                sqlHandler_->updateTable(username, "avatar", avatar);
                responseJson["success"] = true;
                responseJson["newAvatarUrl"] = "avatar/" + avatar;
                generateResponse(json::serialize(responseJson), "application/json");
                logger_.info() << "HttpConn: Upload Avatar " + filename;
            }
            catch (const sql::SQLException& e){
                logger_.error() << "HttpConn: Update User Avatar Failed: " << e.what();
                responseJson["message"] = "Failed to store file infomation in database.";
                responseJson["success"] = false;
                generateResponse(json::serialize(responseJson), "application/json", http::status::internal_server_error);
                return;
            }
        } catch (const std::exception& e) {
            logger_.error() << "HttpConn: Upload Avatar Failed: " << e.what();
        }
    }


    else{
        std::string body = "<html><body><h1>Hello from C++ WebServer!</h1></body></html>";
        generateResponse(body);
        logger_ << "HttpConn: else";
    }
    if (!request_.keep_alive()) {
        close();
        logger_ << "Http closed.";
    }
}

void HttpConn::close(){
    boost::system::error_code ec;
    socket_->shutdown(tcp::socket::shutdown_send, ec);
}

void HttpConn::handleClient(){
    logger_ << "HttpConn: handleClient.";
    try {
        //http::read(*socket_, buffer_, request_);   
        parser_.body_limit(10 * 1024 * 1024); // limit request_size to 10 MB
        boost::beast::http::read(*socket_, buffer_, parser_);
        request_ = parser_.get();
        logger_ << "HttpConn: handleRequest.";
        handleRequest();
    } catch (const boost::system::system_error& e) {
        logger_.error();
        logger_ << std::string("Error in handleClient: ") + e.what();
        logger_.info();
        close();
        logger_ << "Http closed.";
    }
}

std::string urlEncode(const std::string &value) {
    std::ostringstream escaped;
    escaped.fill('0');
    escaped << std::hex;

    for (char c : value) {
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            escaped << c;
        } else {
            escaped << '%' << std::setw(2) << int((unsigned char) c);
        }
    }
    return escaped.str();
}

std::string urlDecode(const std::string& str) {
    std::string result;
    char ch;
    int i, j;
    int length = int(str.length());
    for (i = 0; i < length; i++) {
        if (str[i] == '%') {
            sscanf(str.substr(i + 1, 2).c_str(), "%2x", &j);
            ch = static_cast<char>(j);
            result += ch;
            i = i + 2;
        } else {
            result += str[i];
        }
    }
    return result;
}

// extract the param value from url
std::string getQueryParameter(const std::string& url, const std::string& param) {
    size_t pos = url.find(param + "=");
    if (pos == std::string::npos) return "";

    size_t start = pos + param.length() + 1;  // skip param and '='
    size_t end = url.find("&", start);
    if (end == std::string::npos) end = url.length();

    return url.substr(start, end - start);
}

void HttpConn::sendFile(const std::string& file_path) {
    std::string decoded_path = urlDecode(file_path);
    std::ifstream file(decoded_path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        generateResponse("File not found", "text/plain", http::status::not_found);
        logger_ << "HttpConn: File not found.";
        return;
    }

    // std::ostringstream buffer;
    // buffer << file.rdbuf();
    // std::string fileContent = buffer.str();
    std::streamsize fileSize = file.tellg();
    file.seekg(0, std::ios::beg);
    std::vector<char> fileContent(fileSize);
    if (!file.read(fileContent.data(), fileSize)) {
        generateResponse("Error reading file", "text/plain", http::status::internal_server_error);
        return;
    }
    logger_ << std::string("filesize: ") + std::to_string(fileSize);
    std::string fileContentStr(fileContent.begin(), fileContent.end());

    http::response<http::string_body> res{http::status::ok, 11};
    res.set(http::field::content_type, "application/octet-stream");
    res.set(http::field::content_length, std::to_string(fileContent.size()));
    
    std::string filename = file_path.substr(file_path.find_last_of("/") + 1);
    //std::string utf8Filename = boost::locale::conv::to_utf<char>(filename, "UTF-8");
    //std::string encodedFilename = urlEncode(utf8Filename);
    res.set(http::field::content_disposition, 
        "attachment; filename=\"" + filename + "\"; filename*=UTF-8''" + filename);

    res.body() = std::move(fileContentStr);
    res.prepare_payload();

    try{
        http::write(*socket_, res);
        logger_ << "HttpConn: Sendfile Successed.";
    }
    catch (const std::exception& e){
        logger_.error();
        logger_ << "HttpConn: Sendfile Failed:" << e.what();
    }
    // http::async_write(socket_, res, [&](const boost::system::error_code& ec, std::size_t bytes_transferred) {
    //     if (ec) {
    //         logger_.error();
    //         logger_ << std::string("Error sending file: ") + ec.message();
    //         logger_.info();
    //     }
    // });
}
