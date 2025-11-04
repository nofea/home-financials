#include "family.hpp"
#include <algorithm>

/**
 * @brief Construct a new Family:: Family object
 * 
 * @param name Name of the family
 */
Family::Family(const std::string& name)
    : family_id(0), family_name(name) 
{

}

/**
 * @brief Construct a new Family:: Family object from database
 * 
 * @param id ID of the family
 * @param name Name of the family
 */
Family::Family(uint64_t id, const std::string& name)
    : family_id(id), family_name(name) 
{

}

/**
 * @brief Destroy the Family:: Family object
 * 
 */
Family::~Family() 
{
    // TODO: Cleanup resources
}

/**
 * @brief Adds a member to the family.
 * 
 * @param member The member to be added.
 */
void Family::addMember(const Member& member) 
{
    members.push_back(member);
}

/**
 * @brief Removes a member from the family.
 * 
 * @param member_id The ID of the member to be removed.
 * @return true if the member was removed successfully.
 * @return false if the member could not be found.
 */
bool Family::removeMember(const uint64_t& member_id) 
{
    auto it = std::remove_if(members.begin(), members.end(), [&](const Member& member) {
        return member.getId() == member_id;
    });

    if (it != members.end()) 
    {
        members.erase(it, members.end());
        return true;
    }

    return false;
}

/**
 * @brief Retrieves a member by their ID.
 * 
 * @param member_id The ID of the member to retrieve.
 * @return Members* Pointer to the member if found, nullptr otherwise.
 */
Member* Family::getMember(const uint64_t& member_id) 
{
    auto it = std::find_if(members.begin(), members.end(), [&](const Member& member) {
        return member.getId() == member_id;
    });

    if (it != members.end()) 
    {
        return &(*it);
    }

    return nullptr;
}

/**
 * @brief Get the ID of the family.
 * 
 * @return uint64_t 
 */
uint64_t Family::getId() const 
{
    return family_id;
}

/**
 * @brief Get the name of the family.
 * 
 * @return std::string 
 */
std::string Family::getName() const 
{
    return family_name;
}
/**
 * @brief Get the members of the family.
 * 
 * @return std::vector<Members> 
 */
std::vector<Member> Family::getMembers() const 
{
    return members;
}
