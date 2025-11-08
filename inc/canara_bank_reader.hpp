#pragma once

#include "bank_reader.hpp"
#include <optional>
#include <string>

// Concrete reader for Canara Bank CSV statements. It extracts
// Account Number, Opening Balance and Closing Balance (in paise).
class CanaraBankReader : public BankReader
{
public:
    CanaraBankReader() = default;
    ~CanaraBankReader() override = default;

    // BankReader interface
    std::string bankId() const override { return "canara"; }
    commons::Result parse(std::istream &in) override;

    // BankReader generic accessor
    std::optional<BankReader::BankAccountInfo> extractAccountInfo() const override;

    // Accessors for parsed values. Values are optional until parsing
    // succeeds and the corresponding field is found.
    std::optional<std::string> accountNumber() const { return m_accountNumber; }
    std::optional<long long> openingBalancePaise() const { return m_openingPaise; }
    std::optional<long long> closingBalancePaise() const { return m_closingPaise; }

private:
    std::optional<std::string> m_accountNumber;
    std::optional<long long> m_openingPaise;
    std::optional<long long> m_closingPaise;
};
