#pragma once

#include <string>

/**
 * Abstract interface for I/O operations. This allows TUIManager to be tested
 * by injecting mock I/O instead of using std::cin/cout directly.
 * 
 * This interface defines only the core I/O operations needed by production code.
 */
class IOInterface 
{
public:
    virtual ~IOInterface() = default;

    // Output operations
    virtual void printLine(const std::string& line) = 0;
    virtual void printError(const std::string& error) = 0;

    // Input operations
    virtual bool getLine(std::string& line) = 0;
};

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
