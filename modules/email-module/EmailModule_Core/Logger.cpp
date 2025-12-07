#include "Logger.h"
#include "Utils.h"

#include <fstream>
#include <iostream>

void EmailLogger::Log(const std::string& level, const std::string& msg)
{
    std::ofstream ofs("email.log", std::ios::app);
    if (!ofs.is_open())
    {
        std::cerr << "[LOGGER ERROR] Cannot open email.log" << std::endl;
        return;
    }
    ofs << "[" << GetCurrentTimeString() << "][" << level << "] " << msg << "\n";
}

void EmailLogger::Info(const std::string& msg)
{
    Log("INFO", msg);
}

void EmailLogger::Error(const std::string& msg)
{
    Log("ERROR", msg);
}
