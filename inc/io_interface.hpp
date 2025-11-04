#pragma once

#include <string>
#include <vector>

/**
 * Abstract interface for I/O operations. This allows TUIManager to be tested
 * by injecting mock I/O instead of using std::cin/cout directly.
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
    
    // Utility for tests to pre-load input lines
    virtual void queueInput(const std::vector<std::string>& inputs) = 0;
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
    void queueInput(const std::vector<std::string>&) override {} // No-op for real terminal
};
