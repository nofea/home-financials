#pragma once

#include "reader.hpp"
#include <string>

// Base class for bank-specific readers. Derive individual bank readers
// (e.g. `NatWestReader`, `BarclaysReader`) from this class.
class BankReader : public Reader
{
public:
    virtual ~BankReader() = default;

    // Return a short identifier for the bank this reader handles (e.g.
    // "natwest" or "barclays"). This is useful for selecting a reader
    // implementation at runtime.
    virtual std::string bankId() const = 0;

    // The stream-based parse method is inherited from `Reader` and remains
    // pure virtual; concrete bank readers must implement it.
    virtual commons::Result parse(std::istream &in) override = 0;
};
