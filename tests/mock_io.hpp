#pragma once

#include "io_interface.hpp"
#include <string>
#include <vector>

/**
 * Mock I/O implementation for testing. Captures output and provides
 * pre-programmed input responses.
 */
class MockIO : public IOInterface 
{
public:
    void printLine(const std::string& line) override;
    void printError(const std::string& error) override;
    bool getLine(std::string& line) override;
    void queueInput(const std::vector<std::string>& inputs) override;

    // Test helpers to verify output
    const std::vector<std::string>& getOutput() const { return output; }
    const std::vector<std::string>& getErrors() const { return errors; }
    void clear() { output.clear(); errors.clear(); inputs.clear(); current_input = 0; }

private:
    std::vector<std::string> output;
    std::vector<std::string> errors;
    std::vector<std::string> inputs;
    size_t current_input{0};
};
