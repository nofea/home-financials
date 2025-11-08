#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <optional>

#include <cctype>
#include <cstdlib>
#include <stdexcept>
namespace commons
{
    // Common utility functions and types can be defined here
    enum class Result 
    {
        Ok = 0,
        InvalidInput = 1,
        MaxMembersExceeded = 2,
        NotFound = 3,
        DbError = 4,
    };
    
    // Parse a currency-like string (for example: "Rs.7,43,483.09" or
    // "3,23,527.09") and return the value in paise (1 INR = 100 paise).
    // Returns std::nullopt if the string cannot be parsed.
    inline std::optional<long long> parseMoneyToPaise(const std::string &s)
    {
        std::string filtered;
        bool negative = false;

        for (char ch : s)
        {
            if (ch == '-')
            {
                negative = true;
            }
            if (std::isdigit(static_cast<unsigned char>(ch)))
            {
                filtered.push_back(ch);
            }
            else if (ch == '.')
            {
                filtered.push_back('.');
            }
            // ignore all other characters
        }

        if (filtered.empty())
        {
            return std::nullopt;
        }

        // If there are multiple dots (for example "Rs.7,43,483.09" has a dot after
        // the currency symbol and another for the fractional part), treat the
        // last dot as the decimal separator and remove any earlier dots.
        std::size_t lastDot = filtered.find_last_of('.');
        std::string intpart;
        std::string frac;

        if (lastDot == std::string::npos)
        {
            // No decimal point present
            intpart = filtered;
        }
        else
        {
            // Build intpart by removing any dots before lastDot
            for (std::size_t i = 0; i < lastDot; ++i)
            {
                if (filtered[i] != '.') intpart.push_back(filtered[i]);
            }
            // Build fractional part by removing any dots after lastDot (defensive)
            for (std::size_t i = lastDot + 1; i < filtered.size(); ++i)
            {
                if (filtered[i] != '.') frac.push_back(filtered[i]);
            }
        }

        if (intpart.empty()) intpart = "0";

        // Normalize fractional part to exactly two digits (paise)
        if (frac.size() > 2)
        {
            frac = frac.substr(0, 2);
        }
        while (frac.size() < 2)
        {
            frac.push_back('0');
        }

        std::string combined = intpart + frac;

        try
        {
            long long value = std::stoll(combined);
            return negative ? -value : value;
        }
        catch (const std::exception &)
        {
            return std::nullopt;
        }
    }

} // namespace commons
