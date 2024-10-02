#pragma once

#include <iostream>
#include <fstream>
#include <memory>
#include <cstdarg>
#include <string>

class Logger {

    Logger() = default;
    ~Logger() = default;

    std::string m_logFileName = "log.txt";

public:

    static Logger& Instance() 
    {
        static Logger instance;
        return instance;
    }

    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    static void Log(const std::string& message)
    {
        Instance().appendToLog(message);
    }

    static void Log(const char* format, ...)
    {
        Instance().appendToLog(format);
    }

    void appendToLog(const std::string& message) 
    {
        std::ofstream logFile(m_logFileName, std::ios_base::app);
        if (logFile.is_open()) { logFile << message << std::endl; }
    }

    void appendToLog(const char* format, ...) 
    {
        std::ofstream logFile(m_logFileName, std::ios_base::app);
        if (logFile.is_open()) 
        {
            va_list args;
            va_start(args, format);
            char buffer[1024];
            vsnprintf(buffer, sizeof(buffer), format, args);
            logFile << buffer << std::endl;
            va_end(args);
        }
    }

    void overwriteLog(const std::string& message) 
    {
        std::ofstream logFile(m_logFileName, std::ios_base::trunc);
        if (logFile.is_open()) 
        {
            logFile << message << std::endl;
        }
    }

    void overwriteLog(const char* format, ...) 
    {
        std::ofstream logFile(m_logFileName, std::ios_base::trunc);
        if (logFile.is_open()) 
        {
            va_list args;
            va_start(args, format);
            char buffer[1024];
            vsnprintf(buffer, sizeof(buffer), format, args);
            logFile << buffer << std::endl;
            va_end(args);
        }
    }

    // Set the log file name
    void setLogFileName(const std::string& fileName) 
    {
        m_logFileName = fileName;
    }

private:
};
