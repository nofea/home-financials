#include "home_manager.hpp"

/**
 * @brief Construct a new HomeManager object
 *
 * HomeManager holds an internal StorageManager instance and forwards
 * higher-level operations requested by the UI layer to StorageManager.
 */
HomeManager::HomeManager()
{
	ptr_storage = std::make_unique<StorageManager>();
}


/**
 * @brief Destroy the HomeManager object
 */
HomeManager::~HomeManager()
{
	ptr_storage.reset();
}

/**
 * @brief Add a new family.
 * 
 * @param family Family to add.
 * @return StorageManager::Result 
 */
StorageManager::Result HomeManager::addFamily(const Family &family)
{
	uint64_t id = 0;
	return ptr_storage->saveFamilyDataEx(family, &id);
}

/**
 * @brief Get a family by ID.
 * 
 * @param family_id ID of the family to retrieve.
 * @return std::unique_ptr<Family> 
 */
std::unique_ptr<Family> HomeManager::getFamily(const uint64_t family_id)
{
	Family* f = ptr_storage->getFamilyData(family_id);
	if (!f) return nullptr;
	return std::unique_ptr<Family>(f);
}

/**
 * @brief Update a family's name.
 * 
 * @param family_id ID of the family to update.
 * @param new_name Name to update to.
 * @return StorageManager::Result 
 */
StorageManager::Result HomeManager::updateFamilyName(const uint64_t family_id, const std::string &new_name)
{
	return ptr_storage->updateFamilyDataEx(family_id, new_name);
}

/**
 * @brief Delete a family by ID.
 * 
 * @param family_id ID of the family to delete.
 * @return StorageManager::Result 
 */
StorageManager::Result HomeManager::deleteFamily(const uint64_t family_id)
{
	return ptr_storage->deleteFamilyDataEx(family_id);
}

/**
 * @brief Add a member to a family.
 * 
 * @param member Member to add.
 * @param family_id ID of the family to add the member to.
 * @return StorageManager::Result 
 */
StorageManager::Result HomeManager::addMemberToFamily(const Member &member, const uint64_t family_id)
{
	uint64_t id = 0;
	return ptr_storage->saveMemberDataEx(member, family_id, &id);
}

/**
 * @brief Get a member by ID.
 * 
 * @param member_id ID of the member to retrieve.
 * @return std::unique_ptr<Member> 
 */
std::unique_ptr<Member> HomeManager::getMember(const uint64_t member_id)
{
	Member* m = ptr_storage->getMemberData(member_id);
	if (!m) return nullptr;
	return std::unique_ptr<Member>(m);
}

/**
 * @brief Update a member's details.
 * 
 * @param member_id ID of the member to update.
 * @param new_name Name to update to.
 * @param new_nickname Nickname to update to.
 * @return StorageManager::Result 
 */
StorageManager::Result HomeManager::updateMember(const uint64_t member_id, const std::string &new_name, const std::string &new_nickname)
{
	// Directly delegate to StorageManager which handles partial updates validation
	return ptr_storage->updateMemberDataEx(member_id, new_name, new_nickname);
}

/**
 * @brief Delete a member by ID.
 * 
 * @param member_id ID of the member to delete.
 * @return StorageManager::Result 
 */
StorageManager::Result HomeManager::deleteMember(const uint64_t member_id)
{
	return ptr_storage->deleteMemberDataEx(member_id);
}
