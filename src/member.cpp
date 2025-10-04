#include "member.hpp"

/**
 * @brief Construct a new Member:: Member object
 * 
 */
Member::Member() 
{
    // TODO: Initialize member variables
}

/**
 * @brief Construct a new Member:: Member object
 * 
 * @param id member ID
 * @param name member name
 * @param nickname member nickname
 */
Member::Member(const uint64_t& id, const std::string& name, const std::string& nickname)
    : member_id(id), member_name(name), member_nickname(nickname) 
{

}

/**
 * @brief Destroy the Member:: Member object
 * 
 */
Member::~Member() 
{
    // TODO: Cleanup if necessary
}

/**
 * @brief Get the ID of the member.
 * 
 * @return uint64_t 
 */
uint64_t Member::getId() const 
{
    return member_id;
}

/**
 * @brief Get the name of the member.
 * 
 * @return std::string 
 */
std::string Member::getName() const 
{
    return member_name;
}

/**
 * @brief Get the nickname of the member.
 * 
 * @return std::string 
 */
std::string Member::getNickname() const 
{
    return member_nickname;
}
