#include "GoodAssert.h"
#include "Logger.hpp"
#include <iomanip>
#include <ctime>
#include <iostream>
#include <sstream>
#include <chrono>

std::string CurrentDateTime() 
{
    auto now = std::chrono::system_clock::now();
    std::time_t now_time = std::chrono::system_clock::to_time_t(now);
    std::tm* ptm = std::localtime(&now_time);

    std::ostringstream oss;
    oss << std::put_time(ptm, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

void Assert::ReportFailure(const char * condition, const char * file, int line, const char * msg, ...)
{
    std::cerr << "\nAssertion thrown!\n";

    std::vector<char> messageBuffer(1024, '\0');
    if (msg != nullptr) 
    {
        va_list args;
        va_start(args, msg);
        vsnprintf(messageBuffer.data(), messageBuffer.size(), msg, args);
        va_end(args);
    }

    std::ostringstream ss;
    ss << std::endl;
    ss << "!Assert:   " << condition << std::endl;
    ss << "File:      " << file << std::endl;
    ss << "Message:   " << messageBuffer.data() << std::endl;
    ss << "Line:      " << line << std::endl;
    ss << "Time:      " << CurrentDateTime() << std::endl;

    std::cerr << ss.str() << "\n";
    Logger::Log(ss.str() + "\n");

    exit(-1);
}


