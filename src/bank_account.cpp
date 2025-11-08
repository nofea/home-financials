#include "bank_account.hpp"
#include <sstream>
#include <iomanip>
#include <sqlite3.h>
#include <algorithm>
#include <cctype>

/**
 * @brief Construct a new Bank Account:: Bank Account object
 * 
 */
BankAccount::BankAccount() = default;

/**
 * @brief Construct a new Bank Account:: Bank Account object
 * 
 * @param bankAccountId ID of the bank account
 * @param bankId ID of the bank
 * @param memberId ID of the member
 * @param accountNumber Account number
 * @param openingBalancePaise Opening balance (in paise)
 * @param closingBalancePaise Closing balance (in paise)
 */
BankAccount::BankAccount(uint64_t bankAccountId,
                         uint64_t bankId,
                         uint64_t memberId,
                         const std::string &accountNumber,
                         long long openingBalancePaise,
                         long long closingBalancePaise)
    : bank_account_id{bankAccountId},
      bank_id{bankId},
      member_id{memberId},
      account_number{accountNumber},
      opening_balance_paise{openingBalancePaise},
      closing_balance_paise{closingBalancePaise}
{
}

/**
 * @brief Get the ID of the bank account
 * 
 * @return uint64_t 
 */
uint64_t BankAccount::getId() const
{
    return bank_account_id;
}

/**
 * @brief Set the ID of the bank account
 * 
 * @param id ID to set
 */
void BankAccount::setId(uint64_t id)
{
    bank_account_id = id;
}

/**
 * @brief Get the ID of the bank
 * 
 * @return uint64_t 
 */
uint64_t BankAccount::getBankId() const
{
    return bank_id;
}

/**
 * @brief Set the ID of the bank
 * 
 * @param id ID to set
 */
void BankAccount::setBankId(uint64_t id)
{
    bank_id = id;
}

/**
 * @brief Get the ID of the member
 * 
 * @return uint64_t 
 */
uint64_t BankAccount::getMemberId() const
{
    return member_id;
}

/**
 * @brief Set the ID of the member
 * 
 * @param id ID to set
 */
void BankAccount::setMemberId(uint64_t id)
{
    member_id = id;
}

/**
 * @brief Get the Account Number object
 * 
 * @return const std::string& 
 */
const std::string &BankAccount::getAccountNumber() const
{
    return account_number;
}

/**
 * @brief Set the Account Number object
 * 
 * @param bank_account_str Account number to set
 */
void BankAccount::setAccountNumber(const std::string &bank_account_str)
{
    account_number = bank_account_str;
}

/**
 * @brief Get the Opening Balance (in paise)
 * 
 * @return long long 
 */
long long BankAccount::getOpeningBalancePaise() const
{
    return opening_balance_paise;
}

/**
 * @brief Set the Opening Balance (in paise)
 * 
 * @param opening_balance_paise Opening balance to set
 */
void BankAccount::setOpeningBalancePaise(long long opening_balance_paise)
{
    this->opening_balance_paise = opening_balance_paise;
}

/**
 * @brief Get the Closing Balance (in paise)
 * 
 * @return long long 
 */
long long BankAccount::getClosingBalancePaise() const
{
    return closing_balance_paise;
}

/**
 * @brief Set the Closing Balance (in paise)
 * 
 * @param closing_balance_paise Closing balance to set
 */
void BankAccount::setClosingBalancePaise(long long closing_balance_paise)
{
    this->closing_balance_paise = closing_balance_paise;
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

    BankAccount bank_account(id, bankId, memberId, acct, opening, closing);
    return bank_account;
}

/**
 * @brief Get a human-friendly string representation of the bank account
 * 
 * @return std::string 
 */
std::string BankAccount::toString() const
{
    std::ostringstream ss;
    ss << "BankAccount{id=" << bank_account_id
       << ", bank_id=" << bank_id
       << ", member_id=" << member_id
       << ", account='" << account_number << "'"
       << ", opening_paise=" << opening_balance_paise
       << ", closing_paise=" << closing_balance_paise << "}";
    return ss.str();
}

/**
 * @brief Convert paise (integer) to rupees as double
 * 
 * @param paise Paise amount
 * @return double 
 */
double BankAccount::paiseToRupees(long long paise)
{
    return static_cast<double>(paise) / 100.0;
}

/**
 * @brief Get the Opening Balance (in rupees)
 * 
 * @return double 
 */
double BankAccount::getOpeningBalanceRupees() const
{
    return paiseToRupees(opening_balance_paise);
}

/**
 * @brief Get the Closing Balance (in rupees)
 * 
 * @return double 
 */
double BankAccount::getClosingBalanceRupees() const
{
    return paiseToRupees(closing_balance_paise);
}

/**
 * @brief Normalize the account number by removing spaces and dashes
 * 
 * @param raw String to normalize
 * @return std::string 
 */
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

/**
 * @brief Compare two BankAccount objects for equality
 * 
 * @param other Other BankAccount to compare with
 * @return true If equal
 * @return false If not equal
 */
bool BankAccount::operator==(const BankAccount& other) const
{
    if (bank_account_id != other.bank_account_id)
    {
        return false;
    }

    if (bank_id != other.bank_id)
    {
        return false;
    }
    if (member_id != other.member_id)
    {
        return false;
    }

    // Compare normalized account numbers
    if (normalizeAccountNumber(account_number) != 
        normalizeAccountNumber(other.account_number))
    {
        return false;
    }

    if (opening_balance_paise != other.opening_balance_paise)
    {
        return false;
    }

    if (closing_balance_paise != other.closing_balance_paise)
    {
        return false;
    }

    return true;
}

/**
 * @brief Compare two BankAccount objects for inequality
 * 
 * @param other Other BankAccount to compare with
 * @return true If not equal
 * @return false If equal
 */
bool BankAccount::operator!=(const BankAccount& other) const
{
    return !(*this == other);
}
