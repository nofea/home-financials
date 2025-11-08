#include "terminal_io.hpp"
#include <iostream>

/**
 * @brief Prints a line to the terminal.
 * 
 * @param line The line to print.
 */
void TerminalIO::printLine(const std::string& line) 
{
    std::cout << line << std::endl;
}

/**
 * @brief Prints an error message to the terminal.
 * 
 * @param error The error message to print.
 */
void TerminalIO::printError(const std::string& error) 
{
    std::cerr << error << std::endl;
}

/**
 * @brief Prompts the user for a line of input.
 * 
 * @param line The line to store the input.
 * @return true if input was received, false if an error occurred.
 */
bool TerminalIO::getLine(std::string& line) 
{
    return static_cast<bool>(std::getline(std::cin, line));
}
