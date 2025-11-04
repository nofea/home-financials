#pragma once

#include "commons.hpp"

class Member
{
private:
    uint64_t member_id;
    std::string member_name;
    std::string member_nickname;

    Member() = delete; // Prevent default constructor
    
public:
    // Constructor for creating new members (before saving to DB)
    Member(const std::string& name, const std::string& nickname="");
    
    // Constructor for members retrieved from database (with known ID)
    Member(uint64_t id, const std::string& name, const std::string& nickname="");
    
    virtual ~Member();

    uint64_t getId() const;
    std::string getName() const;
    std::string getNickname() const;
};
