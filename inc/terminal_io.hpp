#pragma once

#include "io_interface.hpp"

/**
 * Standard terminal I/O implementation using cout/cin.
 */
class TerminalIO : public IOInterface 
{
public:
    void printLine(const std::string& line) override;
    void printError(const std::string& error) override;
    bool getLine(std::string& line) override;
};
