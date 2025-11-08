#pragma once

#include "reader.hpp"
#include <optional>
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

    // Generic extracted account info returned by readers after parsing.
    // Implementations should populate this struct when parse() succeeds.
    struct BankAccountInfo
    {
        std::string accountNumber;
        long long openingBalancePaise{0};
        long long closingBalancePaise{0};
    };

    // After parse() has been called, callers can use extractAccountInfo()
    // to obtain parsed account-level data (account number and balances).
    // Returns std::nullopt if the implementation did not parse or the
    // requested fields are not available.
    virtual std::optional<BankAccountInfo> extractAccountInfo() const = 0;
};
