#include "logger.hh"
using std::ifstream;

Logger::Logger() {
    std::string path = ".log";
    this->fp = fopen(path.c_str() ,"a+");
    if (!this->fp) {
        std::cout << "Fail to open log file" << std::endl;
        exit(1);
    }
}
Logger::~Logger() { fclose(this->fp); }

void Logger::info(std::string str) {
    std::string prefix = "[info]";
    fprintf(this->fp, (prefix + str + "\n").c_str(), str.size() + 20);
}

void Logger::error(std::string str) {
    std::string prefix = "[error]";
    fprintf(this->fp, (prefix + str + "\n").c_str(), str.size() + 20);
}

void Logger::panic(std::string str) {
    std::string prefix = "[panic]";
    fprintf(this->fp, (prefix + str + "\n").c_str(), str.size() + 20);
}
