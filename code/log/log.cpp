#include "log.h"

std::mutex Logger::mtx_;

std::ostream& operator<<(std::ostream & os, LogLevel loglevel){
    switch (loglevel){
        case LogLevel::DEBUG: os << "DEBUG: "; break;
        case LogLevel::INFO: os << "INFO: "; break;
        case LogLevel::WARNING: os << "WARNING: "; break;
        case LogLevel::ERROR: os << "ERROR: "; break;
        default: os << "UNKNOWN: "; break;
    }
    return os;
}

Logger::Logger(const std::string filename) : level_(LogLevel::INFO), filename_(filename) {
    logfile_.open(filename, std::ios::app);
    if (!logfile_.is_open()) {
        std::cerr << "Failed to open log file: " << filename << std::endl;
    }
}

Logger::~Logger(){
    if (logfile_.is_open()) {
        logfile_.close();
    }
}

void Logger::setLevel(LogLevel level){
    std::lock_guard<std::mutex> lock(mtx_);
    level_ = level;
}


Logger& Logger::info(){
    setLevel(LogLevel::INFO);
    return *this;
}

Logger& Logger::debug(){
    setLevel(LogLevel::DEBUG);
    return *this;
}

Logger& Logger::warning(){
    setLevel(LogLevel::WARNING);
    return *this;
}

Logger& Logger::error(){
    setLevel(LogLevel::ERROR);
    return *this;
}

Logger& Logger::operator<<(std::ostream& (*manip)(std::ostream&)) {
    std::lock_guard<std::mutex> lock(mtx_);
    if (!logfile_.good()) return *this;
    logfile_ << manip;
    return *this;
}

void Logger::clear(){
    std::lock_guard<std::mutex> lock(mtx_);
    logfile_.close();
    logfile_.open(filename_, std::ios::out | std::ios::trunc);
    logfile_.close();
    logfile_.open(filename_, std::ios::app);
    if (!logfile_.is_open()) {
        std::cerr << "Failed to open log file: " << filename_ << std::endl;
    }
}