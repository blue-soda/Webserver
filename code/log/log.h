#ifndef LOG_HPP
#define LOG_HPP
#include <fstream>
#include <string>
#include <ctime>
#include <mutex>
#include <iostream>
#include <chrono>
#include <iomanip>
#include <ctime>

enum class LogLevel{
    DEBUG,
    INFO, 
    WARNING,
    ERROR,
};

std::ostream& operator<<(std::ostream& os, LogLevel level);

class Logger{
public:
    Logger(const std::string filename = "../log.txt");
    ~Logger();
    Logger& operator<<(std::ostream& (*manip)(std::ostream&));
    void setLevel(LogLevel level);
    void clear();
    Logger& info();
    Logger& debug();
    Logger& error();
    Logger& warning();

    template <typename T>
    void Log(const T& message, LogLevel level){
        std::lock_guard<std::mutex> lock(mtx_);
        if (!logfile_.good()) return;
        auto now = std::chrono::system_clock::now();
        std::time_t now_time = std::chrono::system_clock::to_time_t(now);
        auto now_tm = *std::localtime(&now_time);
        logfile_ << "[" << std::put_time(&now_tm, "%Y-%m-%d %H:%M:%S") << "] ";
        logfile_ << level << ": " << message << std::endl;
    }

    template <typename T>
    Logger& operator<<(const T& message){
        Log(message, level_);
        return *this;
    }
private:
    std::string filename_;
    std::ofstream logfile_;
    static std::mutex mtx_;
    LogLevel level_;
};

#endif