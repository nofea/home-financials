#include "reader_factory.hpp"
#include "canara_bank_reader.hpp"

#include <algorithm>
#include <cctype>
#include <iostream>

namespace 
{
    // Registry of bank name (lowercase) -> factory function
    /**
     * @brief Gets the registry of bank reader factory functions.
     * 
     * @return std::map<std::string, ReaderFactory::FactoryFn>& 
     */
    static std::map<std::string, ReaderFactory::FactoryFn> &registry()
    {
        static std::map<std::string, ReaderFactory::FactoryFn> inst;
        return inst;
    }

    // Mutex to protect registry access
    /**
     * @brief Gets the mutex for protecting registry access.
     * 
     * @return std::mutex& 
     */
    static std::mutex &registryMutex()
    {
        static std::mutex m;
        return m;
    }

    // Helper to lowercase a string for canonical lookups
    /**
     * @brief Converts a string to lowercase.
     * 
     * @param str 
     * @return std::string 
     */
    static std::string toLower(const std::string &str)
    {
        std::string out = str;
        std::transform(out.begin(), out.end(), out.begin(), [](unsigned char c)
            { return std::tolower(c); });

        return out;
    }
}

/**
 * @brief Registers a bank reader factory function.
 * 
 * @param bank_name Name of the bank.
 * @param fn Factory function to register.
 */
void ReaderFactory::registerReader(const std::string &bank_name, FactoryFn fn)
{
    if (bank_name.empty() || !fn)
    {
        return;
    }

    std::string key = toLower(bank_name);
    std::lock_guard<std::mutex> lk(registryMutex());
    registry()[key] = std::move(fn);
}

/**
 * @brief Unregisters a bank reader factory function.
 * 
 * @param bank_name Name of the bank.
 * @return true if unregistered successfully.
 * @return false if the bank was not found.
 */
bool ReaderFactory::unregisterReader(const std::string &bank_name)
{
    std::string key = toLower(bank_name);
    std::lock_guard<std::mutex> lk(registryMutex());
    auto &r = registry();
    auto it = r.find(key);

    if (it == r.end()) 
    {
        return false;
    } 

    r.erase(it);

    return true;
}

/**
 * @brief Creates a bank reader by bank name.
 * 
 * @param bank_name Name of the bank.
 * @return std::unique_ptr<BankReader> 
 */
std::unique_ptr<BankReader> ReaderFactory::createByBankName(const std::string& bank_name)
{
    std::string key = toLower(bank_name);
    std::lock_guard<std::mutex> lk(registryMutex());
    auto &r = registry();
    auto it = r.find(key);

    if (it == r.end()) 
    {
        return nullptr;
    }
    auto ptr = (it->second)();
    if (!ptr)
    {
        std::cerr << "error: factory for bank '" << bank_name << "' failed to create a reader instance." << std::endl;
    }
    return ptr;
}

/**
 * @brief Creates a bank reader by bank ID.
 * 
 * @param storage Storage manager to query bank name.
 * @param bank_id Bank ID.
 * @return std::unique_ptr<BankReader> 
 */
std::unique_ptr<BankReader> ReaderFactory::createByBankId(StorageManager* storage, uint64_t bank_id)
{
    if (!storage)
    {
        return nullptr;
    }

    std::string name;
    auto r = storage->getBankNameById(bank_id, &name);

    if (r != commons::Result::Ok)
    {
        return nullptr;
    }

    return createByBankName(name);
}

/**
 * @brief Lists all registered bank reader factory functions.
 * 
 * @return std::vector<std::string> 
 */
std::vector<std::string> ReaderFactory::listRegistered()
{
    std::lock_guard<std::mutex> lk(registryMutex());
    std::vector<std::string> out;
    out.reserve(registry().size());

    for (const auto &p : registry())
    {
        out.push_back(p.first);
    }

    return out;
}

// Built-in readers should self-register using the REGISTER_BANK_READER macro
// defined in `inc/reader_registration.hpp` from their own translation units.

// Defensive registration: some linkers may drop translation units that only
// contain static registration objects when creating a static archive. To
// ensure built-in readers are always available at runtime, register a few
// known readers from this translation unit too. This is idempotent because
// ReaderFactory::registerReader simply overwrites any existing entry.
namespace 
{
    static bool ensureBuiltInReadersRegistered = []() {
        // Register Canara reader explicitly so it's available even if the
        // Canara TU was dropped by the linker.
        ReaderFactory::registerReader("Canara", [](){ return std::make_unique<CanaraBankReader>(); });
        return true;
    }();
}
