#pragma once

#include "io_interface.hpp"
#include <string>
#include <vector>

/**
 * Extended interface for testable I/O that adds methods for test control.
 * This follows the Interface Segregation Principle by keeping test-specific
 * methods out of the production IOInterface.
 */
class TestableIOInterface : public IOInterface
{
public:
    virtual void queueInput(const std::vector<std::string>& inputs) = 0;
};

/**
 * Mock I/O implementation for testing. Captures output and provides
 * pre-programmed input responses.
 */
class MockIO : public TestableIOInterface 
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
