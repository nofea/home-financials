#include "fd_manager.hpp"
#include <sstream>
#include <vector>

FDManager::FDManager(StorageManager& storage)
    : storage_manager(storage)
{
}

commons::Result FDManager::addFDManual(uint64_t member_id, 
                                       const std::string& bank_name, 
                                       const std::string& fd_number, 
                                       long long amount_paise)
{
    uint64_t bank_id = 0;
    commons::Result res = storage_manager.getBankIdByName(bank_name, &bank_id);
    
    if (res != commons::Result::Ok)
    {
        return res;
    }

    return storage_manager.saveFDEx(bank_id, member_id, fd_number, amount_paise);
}

void FDManager::setTargetMember(uint64_t member_id)
{
    target_member_id = member_id;
}

/**
 * @brief Parses a summary file of FDs.
 * Expected Format (CSV): BankName,FDNumber,AmountPaise
 */
commons::Result FDManager::parse(std::istream &in)
{
    if (target_member_id == 0)
    {
        return commons::Result::InvalidInput;
    }

    std::string line;
    bool at_least_one_success = false;

    while (std::getline(in, line))
    {
        if (line.empty())
        {
            continue;
        }

        std::stringstream ss(line);
        std::string bank_name, fd_number, amount_str;

        if (std::getline(ss, bank_name, ',') &&
            std::getline(ss, fd_number, ',') &&
            std::getline(ss, amount_str, ','))
        {
            try
            {
                long long amount = std::stoll(amount_str);
                commons::Result res = addFDManual(target_member_id, bank_name, fd_number, amount);
                
                if (res == commons::Result::Ok)
                {
                    at_least_one_success = true;
                }
            }
            catch (...)
            {
                // Skip malformed amount rows
                continue;
            }
        }
    }

    return at_least_one_success ? commons::Result::Ok : commons::Result::InvalidInput;
}

long long FDManager::getTotalFDAmountForMember(uint64_t member_id)
{
    auto fds = storage_manager.listFDsOfMember(member_id);
    long long total = 0;

    for (const auto& fd : fds)
    {
        total += fd.amount_paise;
    }

    return total;
}
