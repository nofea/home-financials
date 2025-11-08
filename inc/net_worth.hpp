#pragma once

#include "commons.hpp"
#include "storage_manager.hpp"
#include <cstdint>
#include <memory>

// NetWorth provides helpers to compute net worth (in paise) for a single
// member or for an entire family by summing closing balances stored in
// BankAccounts.
class NetWorth
{
public:
    explicit NetWorth(StorageManager* storage);
    ~NetWorth();

    // Compute net worth for a member (sum of closing balances). Returns
    // commons::Result::Ok on success and writes paise to out_net_worth_paise.
    // Returns NotFound when the member or family does not exist, or DbError
    // on underlying DB failures.
    commons::Result computeMemberNetWorth(const uint64_t member_id, long long* out_net_worth_paise);

    // Compute net worth for a family by summing all members' closing balances.
    commons::Result computeFamilyNetWorth(const uint64_t family_id, long long* out_net_worth_paise);

private:
    StorageManager* storage_ptr{nullptr};
};
