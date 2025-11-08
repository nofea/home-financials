#include "canara_bank_reader.hpp"

#include <sstream>
#include <algorithm>
#include <cctype>

namespace {
// Trim helpers
inline std::string trim(const std::string &str)
{
    size_t init_str_size = 0;

    while (init_str_size < str.size() && std::isspace(static_cast<unsigned char>(str[init_str_size])))
    {
        ++init_str_size;
    }

    size_t end_str_size = str.size();

    while (end_str_size > init_str_size && std::isspace(static_cast<unsigned char>(str[end_str_size-1])))
    {
        --end_str_size;
    }

    return str.substr(init_str_size, end_str_size - init_str_size);
}

// Basic CSV line parser that respects double quotes and returns fields
inline std::vector<std::string> parseCsvLine(const std::string &line)
{
    std::vector<std::string> out;
    std::string cur;
    bool inQuotes = false;

    for (size_t index = 0; index < line.size(); ++index) 
    {
        char character = line[index];

        if (character == '"') 
        {
            if (inQuotes && index + 1 < line.size() && line[index+1] == '"') 
            {
                // escaped quote
                cur.push_back('"');
                ++index;
            } 
            else 
            {
                inQuotes = !inQuotes;
            }
        } 
        else if (character == ',' && !inQuotes) 
        {
            out.push_back(trim(cur));
            cur.clear();
        } 
        else {
            cur.push_back(character);
        }
    }

    out.push_back(trim(cur));

    return out;
}

// Normalize the account number field as seen in the sample which may be
// represented like =""500012456   "" or plain strings. We remove
// any equal signs and double-quote artifacts and trim whitespace.
inline std::string normalizeAccountField(const std::string &str)
{
    std::string t;

    for (char character : str) 
    {
        if (character == '"' || character == '=')
        {
            continue;
        }

        t.push_back(character);
    }

    // trim
    return trim(t);
}
} // namespace

commons::Result CanaraBankReader::parse(std::istream &in)
{
    m_accountNumber.reset();
    m_openingPaise.reset();
    m_closingPaise.reset();

    std::string line;

    while (std::getline(in, line)) 
    {
        auto fields = parseCsvLine(line);
        if (fields.empty())
        {
            continue;
        }

        // Look for key rows used in sample CSV
        if (fields.size() >= 2) 
        {
            const std::string key = fields[0];
            const std::string val = fields[1];

            if (key == "Account Number") 
            {
                m_accountNumber = normalizeAccountField(val);
            } 
            else if (key == "Opening Balance") 
            {
                auto paise = commons::parseMoneyToPaise(val);
                if (paise)
                {
                    m_openingPaise = *paise;
                }
            } 
            else if (key == "Closing Balance") 
            {
                auto paise = commons::parseMoneyToPaise(val);
                if (paise)
                {
                    m_closingPaise = *paise;
                }
            }
        }
    }

    if (!m_accountNumber || !m_openingPaise || !m_closingPaise) 
    {
        return commons::Result::InvalidInput;
    }

    return commons::Result::Ok;
}

std::optional<BankReader::BankAccountInfo> CanaraBankReader::extractAccountInfo() const
{
    if (!m_accountNumber || !m_openingPaise || !m_closingPaise)
    {
        return std::nullopt;
    }

    BankReader::BankAccountInfo info;
    info.accountNumber = *m_accountNumber;
    info.openingBalancePaise = *m_openingPaise;
    info.closingBalancePaise = *m_closingPaise;
    return info;
}
