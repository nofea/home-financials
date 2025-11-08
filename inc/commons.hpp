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
        std::string cleaned;
        bool negative = false;
        bool seenDot = false;
        for (char ch : s) 
        {
            if (ch == '-') 
            {
                negative = true;
            }
            
            if (std::isdigit(static_cast<unsigned char>(ch))) 
            {
                cleaned.push_back(ch);
            } 
            else if (ch == '.' && !seenDot) 
            {
                cleaned.push_back(ch);
                seenDot = true;
            }
            // ignore all other characters (commas, currency symbols, spaces)
        }

        if (cleaned.empty())
        {
            return std::nullopt;
        }

        // Split on dot
        std::size_t pos = cleaned.find('.');
        std::string intpart = (pos == std::string::npos) ? cleaned : cleaned.substr(0, pos);
        std::string frac = (pos == std::string::npos) ? std::string() : cleaned.substr(pos + 1);

        if (intpart.empty())
        {
            intpart = "0";
        }

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
