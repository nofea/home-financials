#pragma once

#include <string>
#include <cstdint>
#include <sqlite3.h>

class BankAccount
{
public:
    BankAccount();

    BankAccount(uint64_t bankAccountId,
                uint64_t bankId,
                uint64_t memberId,
                const std::string &accountNumber,
                long long openingBalancePaise,
                long long closingBalancePaise);

    // Accessors
    uint64_t getId() const;
    void setId(uint64_t id);

    uint64_t getBankId() const;
    void setBankId(uint64_t id);

    uint64_t getMemberId() const;
    void setMemberId(uint64_t id);

    const std::string &getAccountNumber() const;
    void setAccountNumber(const std::string &s);

    long long getOpeningBalancePaise() const;
    void setOpeningBalancePaise(long long v);

    long long getClosingBalancePaise() const;
    void setClosingBalancePaise(long long v);

    // Convenience helpers
    // Create a BankAccount from an sqlite3_stmt row. The row is expected to have
    // columns in the order: BankAccount_ID, Bank_ID, Member_ID, Account_Number,
    // Opening_Balance, Closing_Balance at the provided base column index.
    static BankAccount fromSqliteRow(sqlite3_stmt* stmt, int baseCol = 0);

    // Human-friendly representation for logging/tests
    std::string toString() const;

    // Convert paise (integer) to rupees as double
    static double paiseToRupees(long long paise);

    // Accessors returning balances in rupees (double)
    double getOpeningBalanceRupees() const;
    double getClosingBalanceRupees() const;

    // Equality compares all fields; account numbers are compared normalized.
    bool operator==(const BankAccount& other) const;
    bool operator!=(const BankAccount& other) const;

    // Normalize an account number for comparison/storage (remove spaces, hyphens,
    // convert to uppercase). This is stable and deterministic.
    static std::string normalizeAccountNumber(const std::string &raw);

private:
    uint64_t bank_account_id{0};
    uint64_t bank_id{0};
    uint64_t member_id{0};
    std::string account_number;
    long long opening_balance_paise{0};
    long long closing_balance_paise{0};
};
