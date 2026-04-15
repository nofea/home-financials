#pragma once

#include "reader.hpp"
#include "storage_manager.hpp"
#include <memory>

/**
 * @brief Manages Fixed Deposit operations including manual entry and file parsing.
 * 
 * Complies with REQ-7 to account for FDs across various banks.
 */
class FDManager
{
private:
    StorageManager& storage_manager;
    uint64_t target_member_id{0};

public:
    explicit FDManager(StorageManager& storage);
    virtual ~FDManager() = default;

    // Part 1: Logic for adding FD via TUI
    commons::Result addFDManual(uint64_t member_id, 
                                const std::string& bank_name, 
                                const std::string& fd_number, 
                                long long amount_paise);

    // Part 2: Logic for reading FD summary files
    // Sets the member context for the subsequent parse call
    void setTargetMember(uint64_t member_id);

    // Inherited from Reader: Parses a CSV summary in format: BankName,FDNumber,AmountPaise
    commons::Result parse(std::istream &in) override;

    // Logic for net worth calculation
    long long getTotalFDAmountForMember(uint64_t member_id);
};
