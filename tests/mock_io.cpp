#include "mock_io.hpp"

void MockIO::printLine(const std::string& line) 
{
    output.push_back(line);
}

void MockIO::printError(const std::string& error) 
{
    errors.push_back(error);
}

bool MockIO::getLine(std::string& line) 
{
    if (current_input >= inputs.size()) 
    {
        return false; // simulate EOF
    }
    
    line = inputs[current_input++];
    return true;
}

void MockIO::queueInput(const std::vector<std::string>& new_inputs) 
{
    inputs = new_inputs;
    current_input = 0;
}
