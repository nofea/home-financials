#include "bank_account.hpp"
#include <sstream>
#include <iomanip>
#include <sqlite3.h>
#include <algorithm>
#include <cctype>


BankAccount::BankAccount() = default;

BankAccount::BankAccount(uint64_t bankAccountId,
                         uint64_t bankId,
                         uint64_t memberId,
                         const std::string &accountNumber,
                         long long openingBalancePaise,
                         long long closingBalancePaise)
    : bank_account_id_{bankAccountId},
      bank_id_{bankId},
      member_id_{memberId},
      account_number_{accountNumber},
      opening_balance_paise_{openingBalancePaise},
      closing_balance_paise_{closingBalancePaise}
{
}

uint64_t BankAccount::getId() const
{
    return bank_account_id_;
}

void BankAccount::setId(uint64_t id)
{
    bank_account_id_ = id;
}

uint64_t BankAccount::getBankId() const
{
    return bank_id_;
}

void BankAccount::setBankId(uint64_t id)
{
    bank_id_ = id;
}

uint64_t BankAccount::getMemberId() const
{
    return member_id_;
}

void BankAccount::setMemberId(uint64_t id)
{
    member_id_ = id;
}

const std::string &BankAccount::getAccountNumber() const
{
    return account_number_;
}

void BankAccount::setAccountNumber(const std::string &s)
{
    account_number_ = s;
}

long long BankAccount::getOpeningBalancePaise() const
{
    return opening_balance_paise_;
}

void BankAccount::setOpeningBalancePaise(long long v)
{
    opening_balance_paise_ = v;
}

long long BankAccount::getClosingBalancePaise() const
{
    return closing_balance_paise_;
}

void BankAccount::setClosingBalancePaise(long long v)
{
    closing_balance_paise_ = v;
}

BankAccount BankAccount::fromSqliteRow(sqlite3_stmt* stmt, int baseCol)
{
    // Columns: baseCol + 0 -> BankAccount_ID
    // baseCol + 1 -> Bank_ID
    // baseCol + 2 -> Member_ID
    // baseCol + 3 -> Account_Number
    // baseCol + 4 -> Opening_Balance
    // baseCol + 5 -> Closing_Balance
    uint64_t id = static_cast<uint64_t>(sqlite3_column_int64(stmt, baseCol + 0));
    uint64_t bankId = static_cast<uint64_t>(sqlite3_column_int64(stmt, baseCol + 1));
    uint64_t memberId = static_cast<uint64_t>(sqlite3_column_int64(stmt, baseCol + 2));
    const unsigned char* acctText = sqlite3_column_text(stmt, baseCol + 3);
    std::string acct = acctText ? reinterpret_cast<const char*>(acctText) : std::string();
    long long opening = static_cast<long long>(sqlite3_column_int64(stmt, baseCol + 4));
    long long closing = static_cast<long long>(sqlite3_column_int64(stmt, baseCol + 5));

    BankAccount b(id, bankId, memberId, acct, opening, closing);
    return b;
}

std::string BankAccount::toString() const
{
    std::ostringstream ss;
    ss << "BankAccount{id=" << bank_account_id_
       << ", bank_id=" << bank_id_
       << ", member_id=" << member_id_
       << ", account='" << account_number_ << "'"
       << ", opening_paise=" << opening_balance_paise_
       << ", closing_paise=" << closing_balance_paise_ << "}";
    return ss.str();
}

double BankAccount::paiseToRupees(long long paise)
{
    return static_cast<double>(paise) / 100.0;
}

double BankAccount::getOpeningBalanceRupees() const
{
    return paiseToRupees(opening_balance_paise_);
}

double BankAccount::getClosingBalanceRupees() const
{
    return paiseToRupees(closing_balance_paise_);
}

std::string BankAccount::normalizeAccountNumber(const std::string &raw)
{
    std::string out;
    out.reserve(raw.size());

    for (unsigned char ch : raw)
    {
        if (ch == ' ' || ch == '-' || ch == '\t')
        {
            continue;
        }

        out.push_back(static_cast<char>(std::toupper(ch)));
    }

    return out;
}

bool BankAccount::operator==(const BankAccount& other) const
{
    if (bank_account_id_ != other.bank_account_id_)
    {
        return false;
    }

    if (bank_id_ != other.bank_id_)
    {
        return false;
    }
    if (member_id_ != other.member_id_)
    {
        return false;
    }

    // Compare normalized account numbers
    if (normalizeAccountNumber(account_number_) != 
        normalizeAccountNumber(other.account_number_))
    {
        return false;
    }

    if (opening_balance_paise_ != other.opening_balance_paise_)
    {
        return false;
    }

    if (closing_balance_paise_ != other.closing_balance_paise_)
    {
        return false;
    }

    return true;
}

bool BankAccount::operator!=(const BankAccount& other) const
{
    return !(*this == other);
}
