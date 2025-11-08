#include "reader_factory.hpp"
#include "canara_bank_reader.hpp"

#include <algorithm>
#include <cctype>

namespace {
    // Registry of bank name (lowercase) -> factory function
    static std::map<std::string, ReaderFactory::FactoryFn> &registry()
    {
        static std::map<std::string, ReaderFactory::FactoryFn> inst;
        return inst;
    }

    // Mutex to protect registry access
    static std::mutex &registryMutex()
    {
        static std::mutex m;
        return m;
    }

    // Helper to lowercase a string for canonical lookups
    static std::string toLower(const std::string &s)
    {
        std::string out = s;
        std::transform(out.begin(), out.end(), out.begin(), [](unsigned char c){ return std::tolower(c); });
        return out;
    }
}

void ReaderFactory::registerReader(const std::string &bank_name, FactoryFn fn)
{
    if (bank_name.empty() || !fn) return;
    std::string key = toLower(bank_name);
    std::lock_guard<std::mutex> lk(registryMutex());
    registry()[key] = std::move(fn);
}

bool ReaderFactory::unregisterReader(const std::string &bank_name)
{
    std::string key = toLower(bank_name);
    std::lock_guard<std::mutex> lk(registryMutex());
    auto &r = registry();
    auto it = r.find(key);
    if (it == r.end()) return false;
    r.erase(it);
    return true;
}

std::unique_ptr<BankReader> ReaderFactory::createByBankName(const std::string& bank_name)
{
    std::string key = toLower(bank_name);
    std::lock_guard<std::mutex> lk(registryMutex());
    auto &r = registry();
    auto it = r.find(key);
    if (it == r.end()) return nullptr;
    return (it->second)();
}

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
