#include "net_worth.hpp"

#include <vector>
#include <memory>

NetWorth::NetWorth(StorageManager* storage)
{
    storage_ptr = storage;
}

NetWorth::~NetWorth()
{
}

commons::Result NetWorth::computeMemberNetWorth(const uint64_t member_id, long long* out_net_worth_paise)
{
    if (!out_net_worth_paise)
    {
        return commons::Result::InvalidInput;
    }

    if (!storage_ptr)
    {
        return commons::Result::DbError;
    }

    // Verify member exists (take ownership of the returned raw pointer)
    std::unique_ptr<Member> m(storage_ptr->getMemberData(member_id));

    if (!m)
    {
        return commons::Result::NotFound;
    }

    // Sum closing balances of all bank accounts for the member
    long long total_paise = 0;
    std::vector<StorageManager::BankAccountRow> accounts = storage_ptr->listBankAccountsOfMember(member_id);

    for (const auto &acct : accounts)
    {
        total_paise += acct.closing_balance_paise;
    }

    *out_net_worth_paise = total_paise;
    return commons::Result::Ok;
}


commons::Result NetWorth::computeFamilyNetWorth(const uint64_t family_id, long long* out_net_worth_paise)
{
    if (!out_net_worth_paise)
    {
        return commons::Result::InvalidInput;
    }

    if (!storage_ptr)
    {
        return commons::Result::DbError;
    }

    // Verify family exists (take ownership of the returned raw pointer)
    std::unique_ptr<Family> f(storage_ptr->getFamilyData(family_id));

    if (!f)
    {
        return commons::Result::NotFound;
    }

    long long family_total_paise = 0;

    // Use StorageManager listing of members for this family
    std::vector<Member> members = storage_ptr->listMembersOfFamily(family_id);

    for (const auto &member : members)
    {
        uint64_t member_id = member.getId();
        std::vector<StorageManager::BankAccountRow> accounts = storage_ptr->listBankAccountsOfMember(member_id);

        for (const auto &acct : accounts)
        {
            family_total_paise += acct.closing_balance_paise;
        }
    }

    *out_net_worth_paise = family_total_paise;
    return commons::Result::Ok;
}
