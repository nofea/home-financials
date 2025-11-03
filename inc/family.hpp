#pragma once

#include "commons.hpp"
#include "member.hpp"

class Family
{
private:
    uint64_t family_id;
    std::string family_name;
    std::vector<Member> members;

    Family() = delete; // Prevent default constructor

public:
    Family(const std::string& name);
    virtual ~Family();

    // Add a member to the family
    void addMember(const Member& member);
    // Remove a member from the family by ID
    bool removeMember(const uint64_t& member_id);

    uint64_t getId() const;

    std::string getName() const;

    std::vector<Member> getMembers() const;

    // Get a member by ID
    Member* getMember(const uint64_t& member_id);
};
