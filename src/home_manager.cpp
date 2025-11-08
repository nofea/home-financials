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
 * @return commons::Result 
 */
commons::Result HomeManager::addFamily(const Family &family)
{
	uint64_t id = 0;
	return ptr_storage->saveFamilyDataEx(family, &id);
}

/**
 * @brief Add a new family and retrieve the created ID.
 * 
 * @param family Family to add.
 * @param out_family_id Pointer to store the created family ID.
 * @return commons::Result 
 */
commons::Result HomeManager::addFamily(const Family &family, uint64_t* out_family_id)
{
	return ptr_storage->saveFamilyDataEx(family, out_family_id);
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
 * @return commons::Result 
 */
commons::Result HomeManager::updateFamilyName(const uint64_t family_id, const std::string &new_name)
{
	return ptr_storage->updateFamilyDataEx(family_id, new_name);
}

/**
 * @brief Delete a family by ID.
 * 
 * @param family_id ID of the family to delete.
 * @return commons::Result 
 */
commons::Result HomeManager::deleteFamily(const uint64_t family_id)
{
	return ptr_storage->deleteFamilyDataEx(family_id);
}

/**
 * @brief Add a member to a family.
 * 
 * @param member Member to add.
 * @param family_id ID of the family to add the member to.
 * @return commons::Result 
 */
commons::Result HomeManager::addMemberToFamily(const Member &member, const uint64_t family_id)
{
	// Fast-path defensive check: enforce REQ-3 (max 255 members) before touching the DB.
	bool ok = false;
	uint64_t count = ptr_storage->getMemberCount(family_id, &ok);
	if (ok)
	{
		// Safe cast after validating the business limit. We use uint64_t at
		// the storage layer to match SQLite's 64-bit results; here we
		// validate the domain (REQ-3) and then cast to a narrower unsigned
		// type for local reasoning.
		if (count >= 255)
		{
			return commons::Result::MaxMembersExceeded;
		}
		uint16_t members = static_cast<uint16_t>(count); // safe: count < 255
		(void)members; // keep the variable to document the cast intent
	}

	uint64_t id = 0;
	return ptr_storage->saveMemberDataEx(member, family_id, &id);
}

/**
 * @brief Add a member to a family and retrieve the created ID.
 * 
 * @param member Member to add.
 * @param family_id ID of the family to add the member to.
 * @param out_member_id Pointer to store the created member ID.
 * @return commons::Result 
 */
commons::Result HomeManager::addMemberToFamily(const Member &member, const uint64_t family_id, uint64_t* out_member_id)
{
	// Fast-path defensive check: enforce REQ-3 (max 255 members) before touching the DB.
	bool ok = false;
	uint64_t count = ptr_storage->getMemberCount(family_id, &ok);
	if (ok && count >= 255)
	{
		return commons::Result::MaxMembersExceeded;
	}

	return ptr_storage->saveMemberDataEx(member, family_id, out_member_id);
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
 * @return commons::Result 
 */
commons::Result HomeManager::updateMember(const uint64_t member_id, const std::string &new_name, const std::string &new_nickname)
{
	// Directly delegate to StorageManager which handles partial updates validation
	return ptr_storage->updateMemberDataEx(member_id, new_name, new_nickname);
}

/**
 * @brief Delete a member by ID.
 * 
 * @param member_id ID of the member to delete.
 * @return commons::Result 
 */
commons::Result HomeManager::deleteMember(const uint64_t member_id)
{
	return ptr_storage->deleteMemberDataEx(member_id);
}

/**
 * @brief List all families in the database.
 * 
 * @return std::vector<Family> Vector of families with IDs
 */
std::vector<Family> HomeManager::listFamilies()
{
	return ptr_storage->listFamilies();
}

/**
 * @brief List all members of a specific family.
 * 
 * @param family_id ID of the family whose members to list
 * @return std::vector<Member> Vector of members with IDs
 */
std::vector<Member> HomeManager::listMembersOfFamily(const uint64_t family_id)
{
	return ptr_storage->listMembersOfFamily(family_id);
}
