#include "reader.hpp"

#include <fstream>

// Convenience implementation: open the given file path and call the stream
// based parser. This keeps most derived classes focused on parsing logic
// and not on file I/O.
/**
 * @brief Parses a file.
 * 
 * @param path Path to the file.
 * @return commons::Result 
 */
commons::Result Reader::parseFile(const std::string &path)
{
    std::ifstream in(path, std::ios::binary);

    if (!in.is_open()) 
    {
        return commons::Result::NotFound;
    }
    
    return parse(in);
}
