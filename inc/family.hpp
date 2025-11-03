#pragma once

#include "commons.hpp"
#include "members.hpp"

class Family
{
private:
    uint64_t family_id;
    std::string family_name;
    std::vector<Members> members;

    Family() = delete; // Prevent default constructor

public:
    Family(const std::string& name);
    virtual ~Family();

    // Add a member to the family
    void addMember(const Members& member);
    // Remove a member from the family by ID
    bool removeMember(const uint64_t& member_id);

    uint64_t getId() const;

    std::string getName() const;

    std::vector<Members> getMembers() const;

    // Get a member by ID
    Members* getMember(const uint64_t& member_id);
};
