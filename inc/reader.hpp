#pragma once

#include <istream>
#include <string>
#include "commons.hpp"

// Abstract base class for document readers (bank statements, P&L, etc.).
// Derive from this class and implement `parse(std::istream&)` to
// provide parsing logic for different statement formats.
class Reader
{
public:
    virtual ~Reader() = default;

    // Parse the document from an input stream. Implementations should
    // read from `in` but not close it. Return a commons::Result indicating
    // success or an error code.
    virtual commons::Result parse(std::istream &in) = 0;

    // Convenience helper: open a file and parse it. Returns an error
    // if the file cannot be opened or if parsing fails.
    commons::Result parseFile(const std::string &path);
};
