#include "terminal_io.hpp"
#include <iostream>

void TerminalIO::printLine(const std::string& line) 
{
    std::cout << line << std::endl;
}

void TerminalIO::printError(const std::string& error) 
{
    std::cerr << error << std::endl;
}

bool TerminalIO::getLine(std::string& line) 
{
    return static_cast<bool>(std::getline(std::cin, line));
}
