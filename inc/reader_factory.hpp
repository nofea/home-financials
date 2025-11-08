#pragma once

#include <memory>
#include <string>
#include <functional>
#include <map>
#include <mutex>
#include "bank_reader.hpp"
#include "storage_manager.hpp"

// ReaderFactory creates concrete BankReader instances for a given bank.
// It supports runtime registration of reader creators so new bank readers
// can be added without modifying the factory implementation.
class ReaderFactory
{
public:
    using FactoryFn = std::function<std::unique_ptr<BankReader>()>;

    // Register a reader factory function with a canonical bank name
    // (case-insensitive comparison is performed by the factory).
    static void registerReader(const std::string &bank_name, FactoryFn fn);

    // Unregister a reader (returns true if removed)
    static bool unregisterReader(const std::string &bank_name);

    // Create a reader by bank id. Returns nullptr when no reader exists for the bank.
    static std::unique_ptr<BankReader> createByBankId(StorageManager* storage, uint64_t bank_id);

    // Create a reader by bank name (case-insensitive). Returns nullptr when unsupported.
    static std::unique_ptr<BankReader> createByBankName(const std::string& bank_name);

    // Return a list of registered bank names (lowercased). Useful for UIs
    // to display supported banks.
    static std::vector<std::string> listRegistered();
};
