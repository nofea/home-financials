#include "tui_manager.hpp"
#include <iostream>
#include <string>

/**
 * @brief Print usage information for the application.
 * 
 * @param prog Program name
 */
static void printUsage(const char* prog)
{
    std::cout << "Usage: " << prog << " [--tui] [--help]\n";
    std::cout << "Options:\n";
    std::cout << "  --tui     Launch the terminal-based UI (default)\n";
    std::cout << "  --help    Show this help message\n";
}

/**
 * @brief Entry point of the Home Financials application.
 * 
 * @param argc Number of command line arguments
 * @param argv Array of command line argument strings
 * @return int 
 */
int main(int argc, char** argv)
{
    bool launch_tui = true; // default for now

    // Simple CLI parsing: recognize --tui and --help. Unknown options
    // will default to launching the TUI but print a hint.
    for (int i = 1; i < argc; ++i) 
    {
        std::string arg(argv[i]);
        if (arg == "--help" || arg == "-h") 
        {
            printUsage(argv[0]);
            return 0;
        } 
        else if (arg == "--tui") 
        {
            launch_tui = true;
        } 
        else 
        {
            std::cout << "Unknown option: '" << arg << "' -- defaulting to TUI. Use --help for options." << std::endl;
        }
    }

    if (launch_tui) 
    {
        TUIManager tui;
        tui.run();
        return 0;
    }

    // Placeholder for future UI types; currently unreachable because
    // launch_tui defaults to true, but kept to show planned extensibility.
    std::cout << "No UI selected. Exiting." << std::endl;
    return 0;
}