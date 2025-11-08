#pragma once

#include <cstdint>
#include <string>
#include <vector>
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
    
} // namespace commons
