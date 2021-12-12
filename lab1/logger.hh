#include <fstream>
#include <stdio.h>
#include <string>
#include <iostream>

using std::ofstream;
class Logger {
   private:
    FILE* fp;
   public:
    Logger();
    ~Logger();

    void info(std::string str);
    void error(std::string str);
    void panic(std::string str);
};